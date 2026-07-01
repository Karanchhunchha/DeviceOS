import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "DeviceOS — IoT Fleet Dashboard",
  description:
    "Real-time IoT device fleet management platform. Live telemetry, BLE provisioning, OTA updates, and shadow state synchronization.",
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <head>
        <link rel="preconnect" href="https://fonts.googleapis.com" />
        <link rel="preconnect" href="https://fonts.gstatic.com" crossOrigin="anonymous" />
      </head>
      <body style={{ margin: 0, padding: 0, background: "#030712" }}>{children}</body>
    </html>
  );
}
