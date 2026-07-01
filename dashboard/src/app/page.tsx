"use client";

import React, { useEffect, useState, useRef, useCallback } from "react";

// ── Types ──────────────────────────────────────────────────────────────────────
type DeviceState =
  | "UNPROVISIONED"
  | "BLE_ADVERTISING"
  | "PROVISIONING"
  | "WIFI_CONNECTING"
  | "CLOUD_CONNECTING"
  | "SHADOW_SYNC"
  | "RUNNING"
  | "DISCONNECTED";

interface DeviceShadow {
  device_id: string;
  version: number;
  state: {
    reported?: {
      temperature?: number;
      humidity?: number;
      status?: string;
    };
    desired?: Record<string, unknown>;
  };
  deviceState?: DeviceState;
  lastSeen?: number;
  history?: { temp: number; hum: number; ts: number }[];
}

// ── Constants ──────────────────────────────────────────────────────────────────
const STATE_STEPS: DeviceState[] = [
  "UNPROVISIONED", "BLE_ADVERTISING", "PROVISIONING",
  "WIFI_CONNECTING", "CLOUD_CONNECTING", "SHADOW_SYNC", "RUNNING",
];

const STATE_LABELS: Record<DeviceState, string> = {
  UNPROVISIONED: "Unprovisioned",
  BLE_ADVERTISING: "BLE Advertising",
  PROVISIONING: "Provisioning",
  WIFI_CONNECTING: "Wi-Fi Connecting",
  CLOUD_CONNECTING: "Cloud Connecting",
  SHADOW_SYNC: "Shadow Sync",
  RUNNING: "Running",
  DISCONNECTED: "Disconnected",
};

const BADGE_CLASS: Record<string, string> = {
  online: "badge-online", RUNNING: "badge-online",
  BLE_ADVERTISING: "badge-advertising", PROVISIONING: "badge-advertising",
  WIFI_CONNECTING: "badge-connecting", CLOUD_CONNECTING: "badge-connecting", SHADOW_SYNC: "badge-syncing",
  offline: "badge-offline", DISCONNECTED: "badge-offline", UNPROVISIONED: "badge-offline",
};

// ── Mini Sparkline ─────────────────────────────────────────────────────────────
function Sparkline({ data, color }: { data: number[]; color: string }) {
  if (data.length < 2) return null;
  const min = Math.min(...data), max = Math.max(...data);
  const range = max - min || 1;
  const w = 80, h = 28;
  const pts = data.map((v, i) => {
    const x = (i / (data.length - 1)) * w;
    const y = h - ((v - min) / range) * h;
    return `${x},${y}`;
  }).join(" ");
  return (
    <svg width={w} height={h} viewBox={`0 0 ${w} ${h}`} style={{ display: "block" }}>
      <polyline points={pts} fill="none" stroke={color} strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

// ── State Timeline ─────────────────────────────────────────────────────────────
function StateTimeline({ current }: { current: DeviceState }) {
  const idx = STATE_STEPS.indexOf(current);
  return (
    <div style={{ marginTop: 14 }}>
      {STATE_STEPS.map((s, i) => {
        const cls = i < idx ? "done" : i === idx ? "active" : "pending";
        return (
          <div key={s} className={`state-step ${cls}`}>
            <span style={{ fontSize: 10 }}>
              {i < idx ? "✓" : i === idx ? "▶" : "·"}
            </span>
            {STATE_LABELS[s]}
          </div>
        );
      })}
    </div>
  );
}

// ── Device Card ────────────────────────────────────────────────────────────────
function DeviceCard({ device, onSelect }: { device: DeviceShadow; onSelect: () => void }) {
  const temp = device.state?.reported?.temperature;
  const hum = device.state?.reported?.humidity;
  const status = device.state?.reported?.status || device.deviceState || "DISCONNECTED";
  const badgeClass = BADGE_CLASS[status] || "badge-offline";
  const isOnline = status === "online" || device.deviceState === "RUNNING";
  const dotClass = isOnline ? "green" : status === "CLOUD_CONNECTING" || status === "WIFI_CONNECTING" ? "blue" : "red";
  const ageSec = device.lastSeen ? Math.floor((Date.now() - device.lastSeen) / 1000) : null;

  const tempHistory = (device.history || []).map(h => h.temp);
  const humHistory = (device.history || []).map(h => h.hum);

  return (
    <div className="glass-card animate-fade-in-up" onClick={onSelect}
      style={{ cursor: "pointer", padding: 22, animationDelay: "0ms" }}>
      {/* Header */}
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "flex-start", marginBottom: 16 }}>
        <div>
          <div style={{ display: "flex", alignItems: "center", gap: 8, marginBottom: 4 }}>
            <span className={`live-dot ${dotClass}`} />
            <span style={{ fontSize: 13, fontWeight: 700, color: "var(--foreground)", fontFamily: "'JetBrains Mono', monospace" }}>
              {device.device_id}
            </span>
          </div>
          <span style={{ fontSize: 11, color: "var(--muted)" }}>
            v{device.version || 0} · {ageSec !== null ? `${ageSec}s ago` : "never"}
          </span>
        </div>
        <span className={`badge ${badgeClass}`}>{STATE_LABELS[status as DeviceState] || status}</span>
      </div>

      {/* Metrics */}
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 10, marginBottom: 14 }}>
        <div className="metric-block">
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 4, textTransform: "uppercase", letterSpacing: "0.08em" }}>Temp</div>
          <div style={{ fontSize: 22, fontWeight: 300, color: "var(--accent-blue)", lineHeight: 1 }}>
            {temp !== undefined ? temp.toFixed(1) : "--"}
            <span style={{ fontSize: 12, marginLeft: 2 }}>°C</span>
          </div>
          <Sparkline data={tempHistory} color="var(--accent-blue)" />
        </div>
        <div className="metric-block">
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 4, textTransform: "uppercase", letterSpacing: "0.08em" }}>Humidity</div>
          <div style={{ fontSize: 22, fontWeight: 300, color: "var(--accent-green)", lineHeight: 1 }}>
            {hum !== undefined ? hum.toFixed(1) : "--"}
            <span style={{ fontSize: 12, marginLeft: 2 }}>%</span>
          </div>
          <Sparkline data={humHistory} color="var(--accent-green)" />
        </div>
      </div>

      <StateTimeline current={(device.deviceState || (isOnline ? "RUNNING" : "DISCONNECTED")) as DeviceState} />
    </div>
  );
}

// ── Device Detail Panel ────────────────────────────────────────────────────────
function DeviceDetail({ device, onClose }: { device: DeviceShadow; onClose: () => void }) {
  const tempHistory = (device.history || []).map(h => h.temp);
  const humHistory = (device.history || []).map(h => h.hum);

  return (
    <div className="animate-slide-in" style={{
      position: "fixed", top: 0, right: 0, width: 380, height: "100vh",
      background: "var(--surface)", borderLeft: "1px solid var(--border)",
      zIndex: 50, padding: 28, overflowY: "auto"
    }}>
      <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 24 }}>
        <div>
          <div style={{ fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, marginBottom: 4 }}>
            {device.device_id}
          </div>
          <span style={{ fontSize: 11, color: "var(--muted)" }}>Device Details</span>
        </div>
        <button className="btn-ghost" onClick={onClose} style={{ padding: "6px 12px" }}>✕ Close</button>
      </div>

      <div style={{ marginBottom: 20 }}>
        <div style={{ fontSize: 11, color: "var(--muted)", marginBottom: 10, textTransform: "uppercase", letterSpacing: "0.08em" }}>State Machine</div>
        <StateTimeline current={(device.deviceState || "RUNNING") as DeviceState} />
      </div>

      <div style={{ marginBottom: 20 }}>
        <div style={{ fontSize: 11, color: "var(--muted)", marginBottom: 10, textTransform: "uppercase", letterSpacing: "0.08em" }}>Temperature History</div>
        <div className="metric-block">
          <Sparkline data={tempHistory.length > 0 ? tempHistory : [20, 21, 22, 21, 23, 24, 22]} color="var(--accent-blue)" />
          <div style={{ fontSize: 24, fontWeight: 300, color: "var(--accent-blue)", marginTop: 8 }}>
            {device.state?.reported?.temperature?.toFixed(2) ?? "--"}°C
          </div>
        </div>
      </div>

      <div style={{ marginBottom: 20 }}>
        <div style={{ fontSize: 11, color: "var(--muted)", marginBottom: 10, textTransform: "uppercase", letterSpacing: "0.08em" }}>Humidity History</div>
        <div className="metric-block">
          <Sparkline data={humHistory.length > 0 ? humHistory : [50, 52, 48, 55, 51, 53, 50]} color="var(--accent-green)" />
          <div style={{ fontSize: 24, fontWeight: 300, color: "var(--accent-green)", marginTop: 8 }}>
            {device.state?.reported?.humidity?.toFixed(2) ?? "--"}%
          </div>
        </div>
      </div>

      <div>
        <div style={{ fontSize: 11, color: "var(--muted)", marginBottom: 10, textTransform: "uppercase", letterSpacing: "0.08em" }}>Shadow State</div>
        <pre style={{
          background: "rgba(0,0,0,0.5)", border: "1px solid var(--border)", borderRadius: 10,
          padding: 14, fontSize: 11, fontFamily: "'JetBrains Mono', monospace",
          color: "var(--muted)", overflowX: "auto", whiteSpace: "pre-wrap"
        }}>
          {JSON.stringify(device.state, null, 2)}
        </pre>
      </div>
    </div>
  );
}

// ── Provisioning Panel ────────────────────────────────────────────────────────
function ProvisioningPanel({ onProvision }: { onProvision: (port: number, ssid: string, pw: string, tenant: string) => void }) {
  const [port, setPort] = useState("9001");
  const [ssid, setSsid] = useState("HomeWiFi");
  const [pw, setPw] = useState("password123");
  const [tenant, setTenant] = useState("sim_tenant");
  const [busy, setBusy] = useState(false);
  const [result, setResult] = useState<{ ok: boolean; msg: string } | null>(null);

  const handleProvision = async () => {
    setBusy(true);
    setResult(null);
    try {
      const res = await fetch(`http://localhost:${port}/provision`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ssid, password: pw, tenant_id: tenant }),
        signal: AbortSignal.timeout(5000),
      });
      if (res.ok) {
        setResult({ ok: true, msg: `✓ Device on port ${port} provisioned successfully!` });
        onProvision(Number(port), ssid, pw, tenant);
      } else {
        setResult({ ok: false, msg: `✗ Server returned ${res.status}` });
      }
    } catch (e: unknown) {
      const msg = e instanceof Error ? e.message : "Request failed";
      setResult({ ok: false, msg: `✗ ${msg}` });
    }
    setBusy(false);
  };

  return (
    <div className="provision-panel">
      <div style={{ fontSize: 13, fontWeight: 700, marginBottom: 4, color: "var(--accent-purple)" }}>
        📡 BLE Provisioning
      </div>
      <div style={{ fontSize: 11, color: "var(--muted)", marginBottom: 18 }}>
        Send Wi-Fi credentials to an unprovisioned device over simulated BLE (HTTP local port).
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "80px 1fr 1fr 1fr", gap: 10, marginBottom: 14 }}>
        <div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 6, textTransform: "uppercase" }}>BLE Port</div>
          <input className="dev-input" value={port} onChange={e => setPort(e.target.value)} placeholder="9001" />
        </div>
        <div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 6, textTransform: "uppercase" }}>SSID</div>
          <input className="dev-input" value={ssid} onChange={e => setSsid(e.target.value)} />
        </div>
        <div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 6, textTransform: "uppercase" }}>Password</div>
          <input className="dev-input" type="password" value={pw} onChange={e => setPw(e.target.value)} />
        </div>
        <div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginBottom: 6, textTransform: "uppercase" }}>Tenant</div>
          <input className="dev-input" value={tenant} onChange={e => setTenant(e.target.value)} />
        </div>
      </div>

      <div style={{ display: "flex", alignItems: "center", gap: 14 }}>
        <button className="btn-primary" onClick={handleProvision} disabled={busy}>
          {busy ? "Sending..." : "Provision Device"}
        </button>
        {result && (
          <span style={{ fontSize: 12, color: result.ok ? "var(--accent-green)" : "var(--accent-red)", fontFamily: "'JetBrains Mono', monospace" }}>
            {result.msg}
          </span>
        )}
      </div>
    </div>
  );
}

// ── Stats Bar ─────────────────────────────────────────────────────────────────
function StatsBar({ devices }: { devices: DeviceShadow[] }) {
  const online = devices.filter(d => d.state?.reported?.status === "online").length;
  const total = devices.length;
  const avgTemp = devices.reduce((s, d) => s + (d.state?.reported?.temperature || 0), 0) / (total || 1);
  const avgHum = devices.reduce((s, d) => s + (d.state?.reported?.humidity || 0), 0) / (total || 1);

  return (
    <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 16, marginBottom: 28 }}>
      {[
        { label: "Total Devices", value: total, color: "var(--accent-blue)" },
        { label: "Online", value: online, color: "var(--accent-green)" },
        { label: "Avg Temperature", value: total ? `${avgTemp.toFixed(1)}°C` : "--", color: "var(--accent-blue)" },
        { label: "Avg Humidity", value: total ? `${avgHum.toFixed(1)}%` : "--", color: "var(--accent-green)" },
      ].map(({ label, value, color }) => (
        <div key={label} className="glass-card" style={{ padding: "18px 22px" }}>
          <div style={{ fontSize: 10, color: "var(--muted)", textTransform: "uppercase", letterSpacing: "0.08em", marginBottom: 8 }}>{label}</div>
          <div style={{ fontSize: 28, fontWeight: 300, color }}>{value}</div>
        </div>
      ))}
    </div>
  );
}

// ── Main Page ─────────────────────────────────────────────────────────────────
export default function Home() {
  const [devices, setDevices] = useState<Record<string, DeviceShadow>>({});
  const [wsStatus, setWsStatus] = useState<"connecting" | "connected" | "disconnected">("connecting");
  const [selected, setSelected] = useState<string | null>(null);
  const [subscribeId, setSubscribeId] = useState("");
  const [activeTab, setActiveTab] = useState<"fleet" | "provision">("fleet");
  const [msgCount, setMsgCount] = useState(0);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  const connect = useCallback(() => {
    setWsStatus("connecting");
    const ws = new WebSocket("ws://localhost:8080/v1/console");
    wsRef.current = ws;

    ws.onopen = () => setWsStatus("connected");

    ws.onmessage = (ev) => {
      try {
        const data: DeviceShadow = JSON.parse(ev.data);
        if (!data.device_id) return;
        setMsgCount(c => c + 1);
        setDevices(prev => {
          const existing = prev[data.device_id] || {};
          const temp = data.state?.reported?.temperature;
          const hum = data.state?.reported?.humidity;
          const newHistory = [...(existing.history || [])];
          if (temp !== undefined && hum !== undefined) {
            newHistory.push({ temp, hum, ts: Date.now() });
            if (newHistory.length > 20) newHistory.shift();
          }
          return {
            ...prev,
            [data.device_id]: {
              ...existing,
              ...data,
              lastSeen: Date.now(),
              history: newHistory,
            },
          };
        });
      } catch { /* ignore */ }
    };

    ws.onclose = () => {
      setWsStatus("disconnected");
      reconnectRef.current = setTimeout(connect, 3000);
    };
  }, []);

  useEffect(() => {
    connect();
    return () => {
      wsRef.current?.close();
      if (reconnectRef.current) clearTimeout(reconnectRef.current);
    };
  }, [connect]);

  const subscribeDevice = () => {
    if (wsRef.current?.readyState === WebSocket.OPEN && subscribeId.trim()) {
      wsRef.current.send(JSON.stringify({ action: "subscribe", device_id: subscribeId.trim() }));
    }
  };

  const deviceList = Object.values(devices);
  const selectedDevice = selected ? devices[selected] : null;

  const wsIndicator = {
    connecting: { dot: "blue", label: "Connecting..." },
    connected: { dot: "green", label: "Live" },
    disconnected: { dot: "red", label: "Reconnecting..." },
  }[wsStatus];

  return (
    <div style={{ display: "flex", minHeight: "100vh", position: "relative", zIndex: 1 }}>
      {/* Sidebar */}
      <nav className="sidebar">
        {/* Logo */}
        <div style={{ padding: "0 20px 28px" }}>
          <div style={{ fontSize: 16, fontWeight: 800, letterSpacing: "-0.5px" }}>
            <span style={{ color: "var(--accent-blue)" }}>Device</span>
            <span style={{ color: "var(--foreground)" }}>OS</span>
          </div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginTop: 2 }}>Cloud Platform</div>
        </div>

        <div style={{ padding: "0 20px 8px", fontSize: 10, color: "var(--muted)", textTransform: "uppercase", letterSpacing: "0.1em" }}>Platform</div>
        {([
          ["fleet", "◼ Fleet", "Fleet Overview"],
          ["provision", "◉ Provision", "BLE Provisioning"],
        ] as const).map(([key, icon, label]) => (
          <div key={key} className={`nav-item ${activeTab === key ? "active" : ""}`}
            onClick={() => setActiveTab(key)}>
            <span style={{ fontSize: 12 }}>{icon}</span>
            {label}
          </div>
        ))}

        {/* WS status */}
        <div style={{ marginTop: "auto", padding: "20px 20px 0", borderTop: "1px solid var(--border)" }}>
          <div style={{ display: "flex", alignItems: "center", gap: 8 }}>
            <span className={`live-dot ${wsIndicator.dot}`} />
            <span style={{ fontSize: 11, color: "var(--muted)" }}>{wsIndicator.label}</span>
          </div>
          <div style={{ fontSize: 10, color: "var(--muted)", marginTop: 6 }}>{msgCount} messages received</div>
        </div>
      </nav>

      {/* Main */}
      <main style={{ flex: 1, padding: "32px 36px", overflowY: "auto" }}>
        {/* Header */}
        <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 32 }}>
          <div>
            <h1 style={{ fontSize: 22, fontWeight: 700, letterSpacing: "-0.5px" }}>
              {activeTab === "fleet" ? "Fleet Overview" : "BLE Provisioning"}
            </h1>
            <p style={{ fontSize: 12, color: "var(--muted)", marginTop: 4 }}>
              {activeTab === "fleet"
                ? `${deviceList.length} devices tracked in real time`
                : "Provision new devices over simulated BLE transport"}
            </p>
          </div>

          {activeTab === "fleet" && (
            <div style={{ display: "flex", gap: 10 }}>
              <input
                className="dev-input"
                value={subscribeId}
                onChange={e => setSubscribeId(e.target.value)}
                onKeyDown={e => e.key === "Enter" && subscribeDevice()}
                placeholder="Device ID to subscribe..."
                style={{ width: 240 }}
              />
              <button className="btn-primary" onClick={subscribeDevice}>Subscribe</button>
            </div>
          )}
        </div>

        {activeTab === "fleet" && (
          <>
            <StatsBar devices={deviceList} />

            {deviceList.length === 0 ? (
              <div style={{
                textAlign: "center", padding: "80px 40px",
                border: "2px dashed var(--border)", borderRadius: 20,
              }}>
                <div style={{ fontSize: 40, marginBottom: 16, animation: "float 4s ease-in-out infinite" }}>📡</div>
                <div style={{ fontSize: 16, fontWeight: 600, marginBottom: 8 }}>No devices connected</div>
                <div style={{ fontSize: 13, color: "var(--muted)", maxWidth: 400, margin: "0 auto" }}>
                  Run <code style={{ background: "rgba(255,255,255,0.05)", padding: "2px 6px", borderRadius: 4, fontFamily: "JetBrains Mono" }}>
                    go run ./cli/cmd/deviceos simulate --devices 5
                  </code> to start the fleet simulator.
                </div>
              </div>
            ) : (
              <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fill, minmax(320px, 1fr))", gap: 18 }}>
                {deviceList.map(d => (
                  <DeviceCard key={d.device_id} device={d} onSelect={() => setSelected(d.device_id)} />
                ))}
              </div>
            )}
          </>
        )}

        {activeTab === "provision" && (
          <ProvisioningPanel onProvision={(port) => {
            console.log("Provisioned device on port", port);
          }} />
        )}
      </main>

      {/* Detail Slide Panel */}
      {selectedDevice && (
        <DeviceDetail device={selectedDevice} onClose={() => setSelected(null)} />
      )}
    </div>
  );
}
