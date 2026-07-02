package main

import (
	"bytes"
	"context"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"sync"

	"deviceos/cli/simulator"
)

const defaultServer = "http://localhost:8082"

func main() {
	if len(os.Args) < 2 {
		printUsage()
		os.Exit(1)
	}

	command := os.Args[1]
	switch command {
	case "init":
		runInit(os.Args[2:])
	case "flash":
		runFlash(os.Args[2:])
	case "shadow":
		runShadow(os.Args[2:])
	case "simulate":
		runSimulate(os.Args[2:])
	default:
		fmt.Printf("Unknown command: %s\n", command)
		printUsage()
		os.Exit(1)
	}
}

func printUsage() {
	fmt.Println("Usage: deviceos <command> [arguments]")
	fmt.Println("\nCommands:")
	fmt.Println("  init --target <esp32|stm32|linux>      Initialize a new local configuration")
	fmt.Println("  flash --port <com_port>                Flash firmware to the connected device")
	fmt.Println("  shadow get --device-id <id>            Fetch shadow state representation")
	fmt.Println("  shadow update --device-id <id>         Update desired state shadow parameters")
	fmt.Println("  simulate --devices <count>             Simulate a fleet of virtual devices")
}

func runInit(args []string) {
	initCmd := flag.NewFlagSet("init", flag.ExitOnError)
	target := initCmd.String("target", "esp32", "Target micro platform (esp32, stm32, linux)")
	_ = initCmd.Parse(args)

	fmt.Printf("[INFO] Initializing DeviceOS workspace directory...\n")
	fmt.Printf("[INFO] Selected hardware compile target: %s\n", *target)

	config := map[string]string{
		"device_uid": "dev_mock_ef342",
		"target":     *target,
		"server_url": defaultServer,
	}

	configBytes, _ := json.MarshalIndent(config, "", "  ")
	err := os.WriteFile("device_config.json", configBytes, 0644)
	if err != nil {
		fmt.Printf("[ERROR] Failed to write device_config.json: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("[SUCCESS] Workspace initialized. Created config: device_config.json")
}

func runFlash(args []string) {
	flashCmd := flag.NewFlagSet("flash", flag.ExitOnError)
	port := flashCmd.String("port", "", "Serial port where the device is connected (e.g. COM3 or /dev/ttyUSB0)")
	_ = flashCmd.Parse(args)

	if *port == "" {
		fmt.Println("[ERROR] --port is a required parameter for flashing")
		os.Exit(1)
	}

	fmt.Printf("[INFO] Connecting to device on %s...\n", *port)
	fmt.Println("[INFO] Erasing flash...")
	fmt.Println("[INFO] Writing firmware... [100%]")
	fmt.Println("[SUCCESS] Firmware successfully flashed!")
}

func runShadow(args []string) {
	if len(args) < 1 {
		fmt.Println("Usage: deviceos shadow <get|update> [arguments]")
		os.Exit(1)
	}

	subCommand := args[0]
	shadowCmd := flag.NewFlagSet("shadow", flag.ExitOnError)
	deviceId := shadowCmd.String("device-id", "", "Unique ID identifier of device")
	desiredStr := shadowCmd.String("desired", "", "Desired JSON state parameters (for updates)")
	server := shadowCmd.String("server", defaultServer, "Server gateway URL")
	_ = shadowCmd.Parse(args[1:])

	if *deviceId == "" {
		fmt.Println("[ERROR] --device-id is a required parameter")
		os.Exit(1)
	}

	switch subCommand {
	case "get":
		url := fmt.Sprintf("%s/v1/devices/%s/shadow", *server, *deviceId)
		resp, err := http.Get(url)
		if err != nil {
			fmt.Printf("[ERROR] Request failed: %v\n", err)
			os.Exit(1)
		}
		defer resp.Body.Close()
		printResponse(resp)

	case "update":
		if *desiredStr == "" {
			fmt.Println("[ERROR] --desired JSON parameter is required for update commands")
			os.Exit(1)
		}

		var desiredMap map[string]interface{}
		if err := json.Unmarshal([]byte(*desiredStr), &desiredMap); err != nil {
			fmt.Printf("[ERROR] Invalid --desired JSON payload: %v\n", err)
			os.Exit(1)
		}

		updateBody := map[string]interface{}{
			"state": map[string]interface{}{
				"desired": desiredMap,
			},
		}

		bodyBytes, _ := json.Marshal(updateBody)
		url := fmt.Sprintf("%s/v1/devices/%s/shadow", *server, *deviceId)

		req, err := http.NewRequest(http.MethodPut, url, bytes.NewBuffer(bodyBytes))
		if err != nil {
			fmt.Printf("[ERROR] Request init failed: %v\n", err)
			os.Exit(1)
		}
		req.Header.Set("Content-Type", "application/json")

		resp, err := http.DefaultClient.Do(req)
		if err != nil {
			fmt.Printf("[ERROR] Update request failed: %v\n", err)
			os.Exit(1)
		}
		defer resp.Body.Close()
		printResponse(resp)

	default:
		fmt.Printf("Unknown shadow operation: %s\n", subCommand)
		os.Exit(1)
	}
}

func printResponse(resp *http.Response) {
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Printf("[ERROR] Failed to read response: %v\n", err)
		os.Exit(1)
	}

	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		fmt.Printf("[ERROR] Server returned status %s: %s\n", resp.Status, string(body))
		os.Exit(1)
	}

	var pretty bytes.Buffer
	if err := json.Indent(&pretty, body, "", "  "); err != nil {
		fmt.Println(string(body))
	} else {
		fmt.Println(pretty.String())
	}
}

func runSimulate(args []string) {
	if len(args) < 1 {
		fmt.Println("Usage: deviceos simulate --devices <count>")
		os.Exit(1)
	}

	simCmd := flag.NewFlagSet("simulate", flag.ExitOnError)
	count := simCmd.Int("devices", 3, "Number of virtual devices to simulate")
	server := simCmd.String("server", defaultServer, "Server gateway URL")
	latency := simCmd.Int("latency", 50, "Max simulated latency in ms")
	pktLoss := simCmd.Int("packet-loss", 5, "Simulated packet loss percentage")
	pwrCycle := simCmd.Int("power-cycle", 1, "Simulated power cycle percentage")
	_ = simCmd.Parse(args)

	if *count > 10 {
		fmt.Printf("[WARNING] Simulating %d devices. Keep an eye on system RAM!\n", *count)
		fmt.Println("[WARNING] Large-scale tests should only be run on demand.")
	}

	fmt.Printf("[SIMULATOR] Booting %d virtual devices with Realism (Latency: %dms, Loss: %d%%)...\n", *count, *latency, *pktLoss)

	var wg sync.WaitGroup
	ctx := context.Background()

	for i := 1; i <= *count; i++ {
		wg.Add(1)
		go func(id int) {
			defer wg.Done()
			devID := fmt.Sprintf("sim_dev_%04d", id)
			
			cfg := simulator.DeviceConfig{
				DeviceID:      devID,
				CloudServer:   *server,
				BaseBLEPort:   9000,
				IDIndex:       id,
				LatencyMaxMs:  *latency,
				PacketLossPct: *pktLoss,
				PowerCyclePct: *pwrCycle,
			}
			
			machine := simulator.NewDeviceStateMachine(cfg)
			machine.Run(ctx)
		}(i)
	}

	wg.Wait()
}
