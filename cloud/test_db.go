package main

import (
	"fmt"
	"log"
	"os"

	"deviceos/cloud/pkg/database"
)

func main() {
	dbPath := "test_deviceos.db"
	os.Remove(dbPath)
	db, err := database.InitDB(dbPath)
	if err != nil {
		log.Fatalf("Init failed: %v", err)
	}

	err = db.RegisterDevice("test-device", "tenant1", "token123")
	if err != nil {
		log.Fatalf("Register failed: %v", err)
	}

	dev, err := db.GetDevice("test-device")
	if err != nil {
		log.Fatalf("GetDevice failed: %v", err)
	}
	fmt.Printf("Inserted Device: %+v\n", dev)

	err = db.UpdateShadow("test-device", 2, `{"temp":25}`, `{"temp":22}`)
	if err != nil {
		log.Fatalf("UpdateShadow failed: %v", err)
	}

	shadow, err := db.GetShadow("test-device")
	if err != nil {
		log.Fatalf("GetShadow failed: %v", err)
	}
	fmt.Printf("Updated Shadow: %+v\n", shadow)
	
	fmt.Println("DB TEST PASS")
}
