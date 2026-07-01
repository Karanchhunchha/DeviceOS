package database

import (
	"database/sql"
	"fmt"
	_ "github.com/glebarez/go-sqlite"
)

type DB struct {
	Conn *sql.DB
}

type Device struct {
	ID          string
	TenantID    string
	SecretToken string
	CreatedAt   string
}

type ShadowRecord struct {
	DeviceID  string
	Version   int
	Reported  string // JSON text
	Desired   string // JSON text
	UpdatedAt string
}

// InitDB initializes database schemas
func InitDB(dsn string) (*DB, error) {
	conn, err := sql.Open("sqlite", dsn)
	if err != nil {
		return nil, fmt.Errorf("failed to open sqlite database: %w", err)
	}

	db := &DB{Conn: conn}
	if err := db.migrate(); err != nil {
		conn.Close()
		return nil, err
	}

	return db, nil
}

func (db *DB) migrate() error {
	// Enable foreign key support in SQLite
	_, err := db.Conn.Exec("PRAGMA foreign_keys = ON;")
	if err != nil {
		return fmt.Errorf("failed to enable foreign keys: %w", err)
	}

	// Create devices registry table
	devicesTable := `
	CREATE TABLE IF NOT EXISTS devices (
		id TEXT PRIMARY KEY,
		tenant_id TEXT NOT NULL,
		secret_token TEXT NOT NULL,
		created_at DATETIME DEFAULT CURRENT_TIMESTAMP
	);`
	if _, err := db.Conn.Exec(devicesTable); err != nil {
		return fmt.Errorf("failed to create devices table: %w", err)
	}

	// Create device shadows table
	shadowsTable := `
	CREATE TABLE IF NOT EXISTS device_shadows (
		device_id TEXT PRIMARY KEY,
		version INTEGER NOT NULL DEFAULT 1,
		reported TEXT NOT NULL DEFAULT '{}',
		desired TEXT NOT NULL DEFAULT '{}',
		updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
		FOREIGN KEY(device_id) REFERENCES devices(id) ON DELETE CASCADE
	);`
	if _, err := db.Conn.Exec(shadowsTable); err != nil {
		return fmt.Errorf("failed to create device_shadows table: %w", err)
	}

	return nil
}

func (db *DB) RegisterDevice(id, tenant, token string) error {
	tx, err := db.Conn.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()

	// Upsert device — idempotent so reconnecting devices don't fail
	const insertDev = "INSERT OR IGNORE INTO devices (id, tenant_id, secret_token) VALUES (?, ?, ?);"
	if _, err := tx.Exec(insertDev, id, tenant, token); err != nil {
		return fmt.Errorf("failed to upsert device: %w", err)
	}

	// Create shadow record if it doesn't exist yet
	const insertShadow = "INSERT OR IGNORE INTO device_shadows (device_id, version, reported, desired) VALUES (?, 1, '{}', '{}');"
	if _, err := tx.Exec(insertShadow, id); err != nil {
		return fmt.Errorf("failed to initialize device shadow: %w", err)
	}

	return tx.Commit()
}

func (db *DB) GetDevice(id string) (*Device, error) {
	const query = "SELECT id, tenant_id, secret_token, created_at FROM devices WHERE id = ?;"
	row := db.Conn.QueryRow(query, id)

	var dev Device
	if err := row.Scan(&dev.ID, &dev.TenantID, &dev.SecretToken, &dev.CreatedAt); err != nil {
		if err == sql.ErrNoRows {
			return nil, nil // Not found
		}
		return nil, err
	}
	return &dev, nil
}

func (db *DB) GetShadow(deviceId string) (*ShadowRecord, error) {
	const query = "SELECT device_id, version, reported, desired, updated_at FROM device_shadows WHERE device_id = ?;"
	row := db.Conn.QueryRow(query, deviceId)

	var rec ShadowRecord
	if err := row.Scan(&rec.DeviceID, &rec.Version, &rec.Reported, &rec.Desired, &rec.UpdatedAt); err != nil {
		if err == sql.ErrNoRows {
			return nil, nil // Not found
		}
		return nil, err
	}
	return &rec, nil
}

func (db *DB) UpdateShadow(deviceId string, newVersion int, reported, desired string) error {
	const query = `
	UPDATE device_shadows 
	SET version = ?, reported = ?, desired = ?, updated_at = CURRENT_TIMESTAMP 
	WHERE device_id = ?;`
	
	result, err := db.Conn.Exec(query, newVersion, reported, desired, deviceId)
	if err != nil {
		return err
	}

	rows, err := result.RowsAffected()
	if err != nil {
		return err
	}
	if rows == 0 {
		return fmt.Errorf("no shadow record found for device %s", deviceId)
	}

	return nil
}
