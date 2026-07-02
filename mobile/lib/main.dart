import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:fl_chart/fl_chart.dart';
import 'core/provisioning_transport.dart';
import 'core/http_simulated_provisioner.dart';
import 'core/ble_gatt_provisioner.dart';
import 'core/shadow_client.dart';
import 'core/ble_peripheral_manager.dart';
import 'screens/onboarding_screen.dart';

enum ProvisionMode { simulator, bleScan, peerAdvertise }

void main() {
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ShadowClient(cloudHost: '10.101.104.32:8082')),
      ],
      child: const DeviceOSApp(),
    ),
  );
}

class DeviceOSApp extends StatelessWidget {
  const DeviceOSApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'DeviceOS Companion',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark(useMaterial3: true).copyWith(
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF6366F1),
          brightness: Brightness.dark,
          primary: const Color(0xFF6366F1),
          secondary: const Color(0xFF06B6D4),
          surface: const Color(0xFF1E1B4B),
          background: const Color(0xFF0F172A),
        ),
        scaffoldBackgroundColor: const Color(0xFF0B0F19),
        cardTheme: const CardThemeData(
          color: Color(0xFF1E293B),
          elevation: 4,
          margin: EdgeInsets.symmetric(vertical: 8),
        ),
      ),
      home: const OnboardingScreen(),
    );
  }
}

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  // Provisioning & Cloud controllers
  final _formKey = GlobalKey<FormState>();
  final TextEditingController _targetController = TextEditingController(text: '9001');
  final TextEditingController _ssidController = TextEditingController(text: 'HomeWiFi_2.4G');
  final TextEditingController _passwordController = TextEditingController(text: 'securepassword123');
  final TextEditingController _tenantController = TextEditingController(text: 'tenant_default');
  final TextEditingController _cloudHostController = TextEditingController(text: '10.101.104.32:8082');

  final ScrollController _scrollController = ScrollController();
  final BlePeripheralManager _peripheralManager = BlePeripheralManager();

  ProvisionMode _mode = ProvisionMode.simulator;
  bool _isProvisioning = false;
  String? _selectedDevice;

  @override
  void initState() {
    super.initState();
    // Fetch initial device list on load
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<ShadowClient>().fetchDevices();
    });
  }

  Future<void> _refreshData() async {
    final client = context.read<ShadowClient>();
    await client.fetchDevices();
    if (client.activeDeviceId != null) {
      client.subscribe(client.activeDeviceId!);
    }
  }

  void _showSnackBar(String message, {bool isError = false}) {
    ScaffoldMessenger.of(context).hideCurrentSnackBar();
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: isError ? Colors.redAccent : Colors.green,
        duration: const Duration(seconds: 5),
        behavior: SnackBarBehavior.floating,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final shadowClient = context.watch<ShadowClient>();

    return Scaffold(
      appBar: AppBar(
        title: const Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.developer_board, color: Color(0xFF06B6D4)),
            SizedBox(width: 10),
            Flexible(
              child: Text(
                'DeviceOS Companion',
                style: TextStyle(fontWeight: FontWeight.bold, letterSpacing: 0.8),
                overflow: TextOverflow.ellipsis,
              ),
            ),
          ],
        ),
        backgroundColor: const Color(0xFF0F172A),
        elevation: 0,
        actions: [
          IconButton(
            icon: const Icon(Icons.help_outline, color: Colors.white),
            tooltip: 'How to use this app',
            onPressed: _showHelpDialog,
          ),
          Container(
            margin: const EdgeInsets.only(right: 15),
            child: Row(
              children: [
                Container(
                  width: 10,
                  height: 10,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: shadowClient.isConnected ? Colors.green : Colors.red,
                  ),
                ),
                const SizedBox(width: 8),
                Text(
                  shadowClient.isConnected ? 'Connected' : 'Disconnected',
                  style: const TextStyle(fontSize: 12),
                ),
              ],
            ),
          )
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _refreshData,
        child: SingleChildScrollView(
          physics: const AlwaysScrollableScrollPhysics(),
          controller: _scrollController,
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                // 1. Provisioning Section
                _buildGlassCard(
                  title: 'Device Provisioning',
                  icon: Icons.wifi_protected_setup,
                  color: const Color(0xFF6366F1),
                  child: Form(
                    key: _formKey,
                    child: Column(
                      children: [
                        Row(
                          children: [
                            Expanded(
                              child: ChoiceChip(
                                label: const Text('Simulator Mode (HTTP)'),
                                selected: _mode == ProvisionMode.simulator,
                                onSelected: (val) {
                                  if (val) {
                                    setState(() {
                                      _mode = ProvisionMode.simulator;
                                      _targetController.text = '9001';
                                    });
                                  }
                                },
                              ),
                            ),
                            const SizedBox(width: 10),
                            Expanded(
                              child: ChoiceChip(
                                label: const Text('Real BLE Mode'),
                                selected: _mode == ProvisionMode.bleScan,
                                onSelected: (val) {
                                  if (val) {
                                    setState(() {
                                      _mode = ProvisionMode.bleScan;
                                      _targetController.text = '4B:50:AA:BB:CC:DD';
                                    });
                                  }
                                },
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 10),
                        Row(
                          children: [
                            Expanded(
                              child: ChoiceChip(
                                label: const Text('Host Peer Mode (Advertise)'),
                                selected: _mode == ProvisionMode.peerAdvertise,
                                onSelected: (val) {
                                  if (val) {
                                    setState(() {
                                      _mode = ProvisionMode.peerAdvertise;
                                      _targetController.text = 'Peer Mode Active';
                                    });
                                  }
                                },
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 15),
                        TextFormField(
                          controller: _targetController,
                          enabled: _mode != ProvisionMode.peerAdvertise,
                          decoration: InputDecoration(
                            labelText: _mode == ProvisionMode.simulator ? 'Device Local Port / Host' : (_mode == ProvisionMode.bleScan ? 'Device BLE Address / MAC' : 'Advertising...'),
                            prefixIcon: Icon(_mode == ProvisionMode.simulator ? Icons.settings_ethernet : Icons.bluetooth),
                            border: const OutlineInputBorder(),
                          ),
                          validator: (value) {
                            if (_mode == ProvisionMode.peerAdvertise) return null;
                            if (value == null || value.trim().isEmpty) return 'Field is required';
                            if (_mode == ProvisionMode.simulator) {
                              // Basic regex for IPv4:Port or just Port
                              final regex = RegExp(r'^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d{1,5})|(\d{1,5})$');
                              if (!regex.hasMatch(value.trim())) return 'Must be a valid Port (e.g. 9001) or IP:Port';
                            } else {
                              final regex = RegExp(r'^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$');
                              if (!regex.hasMatch(value.trim())) return 'Invalid MAC address format';
                            }
                            return null;
                          },
                        ),
                        const SizedBox(height: 12),
                        TextFormField(
                          controller: _ssidController,
                          decoration: const InputDecoration(
                            labelText: 'Wi-Fi SSID',
                            prefixIcon: Icon(Icons.wifi),
                            border: OutlineInputBorder(),
                          ),
                          validator: (value) => (value == null || value.trim().isEmpty) ? 'SSID required' : null,
                        ),
                        const SizedBox(height: 12),
                        TextFormField(
                          controller: _passwordController,
                          obscureText: true,
                          decoration: const InputDecoration(
                            labelText: 'Wi-Fi Password',
                            prefixIcon: Icon(Icons.lock),
                            border: OutlineInputBorder(),
                          ),
                          validator: (value) => (value == null || value.length < 8) ? 'Password must be at least 8 characters' : null,
                        ),
                        const SizedBox(height: 12),
                        TextFormField(
                          controller: _tenantController,
                          decoration: const InputDecoration(
                            labelText: 'Tenant ID',
                            prefixIcon: Icon(Icons.corporate_fare),
                            border: OutlineInputBorder(),
                          ),
                          validator: (value) => (value == null || value.trim().isEmpty) ? 'Tenant ID required' : null,
                        ),
                        const SizedBox(height: 16),
                        ElevatedButton(
                          style: ElevatedButton.styleFrom(
                            backgroundColor: const Color(0xFF6366F1),
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(vertical: 14),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(8),
                            ),
                          ),
                          onPressed: _isProvisioning ? null : _handleProvision,
                          child: _isProvisioning
                              ? const SizedBox(
                                  height: 24,
                                  width: 24,
                                  child: CircularProgressIndicator(color: Colors.white, strokeWidth: 3),
                                )
                              : Text(
                                  _mode == ProvisionMode.peerAdvertise
                                      ? (_peripheralManager.isAdvertising ? 'Stop Advertising' : 'Start BLE Advertisement')
                                      : 'Provision Wireless Network',
                                  style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 16),
                                ),
                        ),
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 16),

                // 2. Cloud Subscription Controls (Fleet View)
                _buildGlassCard(
                  title: 'Device Shadow Synchronization',
                  icon: Icons.sync,
                  color: const Color(0xFF06B6D4),
                  child: Column(
                    children: [
                      TextFormField(
                        controller: _cloudHostController,
                        decoration: const InputDecoration(
                          labelText: 'Cloud Server Gateway (IP:Port)',
                          prefixIcon: Icon(Icons.cloud),
                          border: OutlineInputBorder(),
                        ),
                      ),
                      const SizedBox(height: 12),
                      Row(
                        children: [
                          Expanded(
                            child: DropdownButtonFormField<String>(
                              decoration: const InputDecoration(
                                labelText: 'Select Device from Fleet',
                                prefixIcon: Icon(Icons.developer_board),
                                border: OutlineInputBorder(),
                              ),
                              value: _selectedDevice != null && shadowClient.devices.contains(_selectedDevice)
                                  ? _selectedDevice
                                  : (shadowClient.devices.isNotEmpty ? shadowClient.devices.first : null),
                              items: shadowClient.devices.map((deviceId) {
                                return DropdownMenuItem<String>(
                                  value: deviceId,
                                  child: Text(deviceId, overflow: TextOverflow.ellipsis),
                                );
                              }).toList(),
                              onChanged: (value) {
                                setState(() {
                                  _selectedDevice = value;
                                });
                              },
                              hint: Text(shadowClient.isLoadingDevices ? 'Loading fleet...' : 'No devices found'),
                            ),
                          ),
                          const SizedBox(width: 10),
                          ElevatedButton(
                            style: ElevatedButton.styleFrom(
                              backgroundColor: const Color(0xFF06B6D4),
                              foregroundColor: Colors.black,
                              padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 20),
                            ),
                            onPressed: () async {
                              shadowClient.updateHost(_cloudHostController.text.trim());
                              
                              // Automatically fetch devices if the list is empty
                              if (shadowClient.devices.isEmpty) {
                                await shadowClient.fetchDevices();
                              }
                              
                              final targetDevice = _selectedDevice ?? 
                                  (shadowClient.devices.isNotEmpty ? shadowClient.devices.first : null);
                              if (targetDevice != null) {
                                shadowClient.subscribe(targetDevice);
                              } else {
                                _showSnackBar('No devices found on this server. Is the simulator running?', isError: true);
                              }
                            },
                            child: const Text('Sync', style: TextStyle(fontWeight: FontWeight.bold)),
                          ),
                        ],
                      ),
                      if (shadowClient.error != null) ...[
                        const SizedBox(height: 10),
                        Material(
                          color: Colors.transparent,
                          child: ExpansionTile(
                            title: const Text('Connection Error - Show Details', style: TextStyle(color: Colors.redAccent, fontSize: 14)),
                            iconColor: Colors.redAccent,
                            collapsedIconColor: Colors.redAccent,
                            children: [
                              Text(
                                shadowClient.error!,
                                style: const TextStyle(color: Colors.redAccent, fontSize: 12),
                              ),
                              if (shadowClient.error!.contains('Timeout') || shadowClient.error!.contains('timed out'))
                                const Padding(
                                  padding: EdgeInsets.only(top: 8.0),
                                  child: Text('Device not found. Make sure it is powered on and in range, or the IP is reachable.', 
                                    style: TextStyle(color: Colors.orange, fontWeight: FontWeight.bold)),
                                )
                            ],
                          ),
                        )
                      ]
                    ],
                  ),
                ),
                const SizedBox(height: 16),

                // 3. Telemetry Twin View
                if (shadowClient.shadow != null)
                  _buildLiveTelemetry(shadowClient.shadow!, shadowClient)
                else
                  _buildGlassCard(
                    title: 'Live Telemetry Monitor',
                    icon: Icons.analytics,
                    color: Colors.grey,
                    child: const Center(
                      child: Padding(
                        padding: EdgeInsets.symmetric(vertical: 24.0),
                        child: Text('Provide device credentials or enter device ID to start telemetry sync.'),
                      ),
                    ),
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  void _showHelpDialog() {
    showDialog(
      context: context,
      builder: (ctx) {
        return AlertDialog(
          backgroundColor: const Color(0xFF1E293B),
          title: const Row(
            children: [
              Icon(Icons.help_outline, color: Color(0xFF06B6D4)),
              SizedBox(width: 10),
              Text('How to Use This App', style: TextStyle(color: Colors.white)),
            ],
          ),
          content: const SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                Text("1. Simulator Mode (HTTP)", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
                Text("Use this to test the flow without any hardware. The app sends provisioning data over HTTP to a local Go CLI simulator. Ensure the Simulator CLI is running.", style: TextStyle(color: Colors.grey)),
                SizedBox(height: 12),
                Text("2. Real BLE Mode", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
                Text("Use this with a real ESP32 device. Enter the BLE MAC address of the device, and the app will connect via Bluetooth to provision Wi-Fi credentials.", style: TextStyle(color: Colors.grey)),
                SizedBox(height: 12),
                Text("3. Host Peer Mode (Advertise)", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
                Text("No hardware? Use two phones! Put Phone A in this mode to broadcast a BLE signal. Put Phone B in 'Real BLE Mode' to scan and connect to Phone A.", style: TextStyle(color: Colors.grey)),
                SizedBox(height: 12),
                Text("Device Shadow Synchronization", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
                Text("Once provisioned, the device connects to the cloud. Enter the Cloud Server IP here, select your device, and hit Sync to see live temperature/humidity telemetry.", style: TextStyle(color: Colors.grey)),
              ],
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(ctx).pop(),
              child: const Text('Got it', style: TextStyle(color: Color(0xFF06B6D4))),
            ),
          ],
        );
      },
    );
  }

  Future<void> _handleProvision() async {
    if (_mode == ProvisionMode.peerAdvertise) {
      try {
        if (_peripheralManager.isAdvertising) {
          await _peripheralManager.stopAdvertising();
          _showSnackBar('BLE Advertisement Stopped.');
        } else {
          await _peripheralManager.startAdvertising();
          _showSnackBar('BLE Advertisement Started. Ready for Peer connection.');
        }
        setState(() {}); // refresh button state
      } catch (e) {
        _showSnackBar('Failed to advertise: $e', isError: true);
      }
      return;
    }

    if (!_formKey.currentState!.validate()) {
      return;
    }

    setState(() {
      _isProvisioning = true;
    });

    final target = _targetController.text.trim();
    final ssid = _ssidController.text.trim();
    final password = _passwordController.text.trim();
    final tenantId = _tenantController.text.trim();

    final ProvisioningTransport transport =
        _mode == ProvisionMode.simulator ? HttpSimulatedProvisioner() : BleGattProvisioner();

    try {
      await transport.provision(
        target: target,
        ssid: ssid,
        password: password,
        tenantId: tenantId,
      );
      
      _showSnackBar('Provisioning succeeded! Device now connecting to Wi-Fi.');
      
      // Auto-scroll to sync section
      _scrollController.animateTo(
        350.0, 
        duration: const Duration(milliseconds: 500), 
        curve: Curves.easeOut,
      );
      
      // Auto register/subscribe to the matching simulated device after a delay
      if (_mode == ProvisionMode.simulator) {
        Future.delayed(const Duration(seconds: 4), () async {
          if (mounted) {
            final client = context.read<ShadowClient>();
            await client.fetchDevices();
            // Automatically select the newly provisioned device (e.g. sim_dev_0001)
            if (client.devices.isNotEmpty) {
              setState(() {
                _selectedDevice = client.devices.last; // Assume the newest is last, or simply select any
              });
              client.subscribe(_selectedDevice!);
            }
          }
        });
      }
    } catch (e) {
      final msg = e.toString();
      _showSnackBar(
        msg.contains('Timeout') || msg.contains('timed out') 
            ? 'Connection Timed Out. Ensure device is reachable/advertising.' 
            : 'Provisioning failed: $e',
        isError: true,
      );
    } finally {
      if (mounted) {
        setState(() {
          _isProvisioning = false;
        });
      }
    }
  }

  Widget _buildGlassCard({
    required String title,
    required IconData icon,
    required Color color,
    required Widget child,
  }) {
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFF1E293B).withOpacity(0.8),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: color.withOpacity(0.3), width: 1.5),
      ),
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            children: [
              Icon(icon, color: color, size: 24),
              const SizedBox(width: 10),
              Expanded(
                child: Text(
                  title,
                  style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
                ),
              ),
            ],
          ),
          const Divider(height: 24, color: Colors.grey),
          child,
        ],
      ),
    );
  }

  Widget _buildLiveTelemetry(DeviceShadowModel shadow, ShadowClient client) {
    final temp = shadow.reported['temperature'] ?? 0.0;
    final humidity = shadow.reported['humidity'] ?? 0.0;
    final status = shadow.reported['status'] ?? 'unknown';
    
    // Desired state configurations
    final relayState = shadow.desired['relay_state'] ?? shadow.reported['relay_state'] ?? false;

    return Column(
      children: [
        _buildGlassCard(
          title: 'Live Digital Twin: ${shadow.deviceId}',
          icon: Icons.bolt,
          color: const Color(0xFFFF007A),
          child: Column(
            children: [
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  _buildMetric('Temperature', '${(temp as num).toStringAsFixed(1)}°C', Icons.thermostat, Colors.orange, client.tempHistory),
                  _buildMetric('Humidity', '${(humidity as num).toStringAsFixed(1)}%', Icons.water_drop, Colors.blue, client.humidityHistory),
                ],
              ),
              const SizedBox(height: 16),
              const Divider(color: Colors.grey),
              const SizedBox(height: 8),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text('Device Network Status', style: TextStyle(fontWeight: FontWeight.w500)),
                  Chip(
                    label: Text(status.toUpperCase()),
                    backgroundColor: status == 'online' ? Colors.green.withOpacity(0.2) : Colors.red.withOpacity(0.2),
                    labelStyle: TextStyle(
                      color: status == 'online' ? Colors.green : Colors.red,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
        const SizedBox(height: 16),
        _buildGlassCard(
          title: 'Device Shadow Configurations (Desired State)',
          icon: Icons.tune,
          color: Colors.lightGreenAccent,
          child: Column(
            children: [
              Material(
                color: Colors.transparent,
                child: SwitchListTile(
                  title: const Text('Desired Relay state (LED Switch)'),
                  subtitle: const Text('Update Desired state via Shadow MQTT/Websocket channel'),
                  value: relayState,
                  onChanged: (bool val) {
                    client.updateDesiredState({'relay_state': val});
                  },
                ),
              ),
              const SizedBox(height: 10),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text('Shadow Document Version:'),
                  Text(
                    'v${shadow.version}',
                    style: const TextStyle(fontFamily: 'monospace', fontWeight: FontWeight.bold),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildMetric(String title, String value, IconData icon, Color color, List<double> history) {
    return Expanded(
      child: Column(
        children: [
          Icon(icon, color: color, size: 36),
          const SizedBox(height: 8),
          Text(title, style: const TextStyle(color: Colors.grey, fontSize: 14)),
          const SizedBox(height: 4),
          Text(value, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 24)),
          const SizedBox(height: 8),
          if (history.length > 1)
            SizedBox(
              height: 40,
              child: LineChart(
                LineChartData(
                  minX: 0,
                  maxX: (history.length - 1).toDouble(),
                  minY: history.reduce((a, b) => a < b ? a : b) - 2,
                  maxY: history.reduce((a, b) => a > b ? a : b) + 2,
                  gridData: FlGridData(show: false),
                  titlesData: FlTitlesData(show: false),
                  borderData: FlBorderData(show: false),
                  lineBarsData: [
                    LineChartBarData(
                      spots: history.asMap().entries.map((e) => FlSpot(e.key.toDouble(), e.value)).toList(),
                      isCurved: true,
                      color: color,
                      barWidth: 2,
                      isStrokeCapRound: true,
                      dotData: FlDotData(show: false),
                      belowBarData: BarAreaData(
                        show: true,
                        color: color.withOpacity(0.2),
                      ),
                    ),
                  ],
                ),
              ),
            )
          else
            const SizedBox(height: 40, child: Center(child: Text('...', style: TextStyle(color: Colors.grey)))),
        ],
      ),
    );
  }
}
