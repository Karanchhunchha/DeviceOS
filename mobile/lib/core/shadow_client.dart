import 'dart:async';
import 'dart:convert';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:web_socket_channel/web_socket_channel.dart';

class DeviceShadowModel {
  final String deviceId;
  final int version;
  final Map<String, dynamic> reported;
  final Map<String, dynamic> desired;
  final Map<String, dynamic> delta;

  DeviceShadowModel({
    required this.deviceId,
    required this.version,
    required this.reported,
    required this.desired,
    required this.delta,
  });

  factory DeviceShadowModel.fromJson(Map<String, dynamic> json) {
    final state = json['state'] as Map<String, dynamic>? ?? {};
    return DeviceShadowModel(
      deviceId: json['device_id'] ?? '',
      version: json['version'] ?? 0,
      reported: state['reported'] as Map<String, dynamic>? ?? {},
      desired: state['desired'] as Map<String, dynamic>? ?? {},
      delta: state['delta'] as Map<String, dynamic>? ?? {},
    );
  }
}

class ShadowClient extends ChangeNotifier {
  String cloudHost; // e.g. "10.101.104.32:8082"
  WebSocketChannel? _channel;
  StreamSubscription? _subscription;
  Timer? _reconnectTimer;
  int _reconnectAttempts = 0;
  
  String? _activeDeviceId;
  DeviceShadowModel? _shadow;
  bool _isConnected = false;
  String? _error;
  
  List<String> _devices = [];
  bool _isLoadingDevices = false;

  final List<double> tempHistory = [];
  final List<double> humidityHistory = [];
  final int _maxHistory = 10;

  ShadowClient({this.cloudHost = '10.101.104.32:8082'});

  void updateHost(String newHost) {
    if (cloudHost != newHost) {
      cloudHost = newHost;
      if (_activeDeviceId != null) {
        subscribe(_activeDeviceId!);
      }
    }
  }

  DeviceShadowModel? get shadow => _shadow;
  bool get isConnected => _isConnected;
  String? get error => _error;
  String? get activeDeviceId => _activeDeviceId;
  List<String> get devices => _devices;
  bool get isLoadingDevices => _isLoadingDevices;

  /// Fetch list of registered devices from the cloud
  Future<void> fetchDevices() async {
    _isLoadingDevices = true;
    notifyListeners();
    
    try {
      final url = Uri.parse('http://$cloudHost/v1/devices');
      final response = await http.get(url).timeout(const Duration(seconds: 5));
      
      if (response.statusCode == 200) {
        final data = jsonDecode(response.body);
        final devs = data['devices'] as List<dynamic>? ?? [];
        _devices = devs.map((e) => e.toString()).toList();
        _error = null;
      } else {
        _error = 'Failed to fetch devices: ${response.statusCode}';
      }
    } catch (e) {
      _error = 'Error fetching devices: $e';
    } finally {
      _isLoadingDevices = false;
      notifyListeners();
    }
  }

  /// Subscribe to a device shadow using WebSocket console.
  void subscribe(String deviceId) {
    if (_activeDeviceId == deviceId && _isConnected) return;
    
    disconnect();
    _activeDeviceId = deviceId;
    _reconnectAttempts = 0;
    
    _connectWebSocket();
  }
  
  void _connectWebSocket() {
    if (_activeDeviceId == null) return;
    
    final wsUrl = Uri.parse('ws://$cloudHost/v1/console');
    _error = null;
    notifyListeners();

    try {
      _channel = WebSocketChannel.connect(wsUrl);
      _isConnected = true;
      _reconnectAttempts = 0;
      _reconnectTimer?.cancel();

      // Subscribe payload
      final subPayload = {
        'action': 'subscribe',
        'device_id': _activeDeviceId,
      };
      _channel!.sink.add(jsonEncode(subPayload));

      _subscription = _channel!.stream.listen(
        (data) {
          try {
            final decoded = jsonDecode(data as String);
            _shadow = DeviceShadowModel.fromJson(decoded);
            _error = null;
            
            // Update history arrays for sparklines
            final temp = (_shadow!.reported['temperature'] ?? 0.0) as num;
            final humidity = (_shadow!.reported['humidity'] ?? 0.0) as num;
            
            tempHistory.add(temp.toDouble());
            if (tempHistory.length > _maxHistory) tempHistory.removeAt(0);
            
            humidityHistory.add(humidity.toDouble());
            if (humidityHistory.length > _maxHistory) humidityHistory.removeAt(0);
            
            notifyListeners();
          } catch (e) {
            if (kDebugMode) print('Error parsing shadow data: $e');
          }
        },
        onError: (err) {
          _error = 'WebSocket error: $err';
          _handleDisconnect();
        },
        onDone: () {
          _handleDisconnect();
        },
      );
    } catch (e) {
      _error = 'Failed to connect: $e';
      _handleDisconnect();
    }
  }
  
  void _handleDisconnect() {
    _isConnected = false;
    _subscription?.cancel();
    _channel = null;
    notifyListeners();
    
    _scheduleReconnect();
  }
  
  void _scheduleReconnect() {
    if (_activeDeviceId == null) return;
    
    _reconnectTimer?.cancel();
    
    // Exponential backoff: 2^attempts seconds, max 30 seconds
    final delaySeconds = min(pow(2, _reconnectAttempts).toInt(), 30);
    _reconnectAttempts++;
    
    if (kDebugMode) print('WebSocket disconnected. Reconnecting in $delaySeconds seconds...');
    
    _reconnectTimer = Timer(Duration(seconds: delaySeconds), () {
      if (_activeDeviceId != null && !_isConnected) {
        _connectWebSocket();
      }
    });
  }

  /// Update the desired state of the device shadow via HTTP PUT API.
  Future<void> updateDesiredState(Map<String, dynamic> desiredPatch) async {
    if (_activeDeviceId == null) return;
    
    final url = Uri.parse('http://$cloudHost/v1/devices/$_activeDeviceId/shadow');
    
    try {
      final response = await http.put(
        url,
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'state': {
            'desired': desiredPatch,
          }
        }),
      );

      if (response.statusCode != 200) {
        throw Exception('Failed to update desired shadow state: ${response.body}');
      }
    } catch (e) {
      _error = 'HTTP Shadow Update error: $e';
      notifyListeners();
      rethrow;
    }
  }

  void disconnect() {
    _reconnectTimer?.cancel();
    _subscription?.cancel();
    _channel?.sink.close();
    _channel = null;
    _isConnected = false;
    _shadow = null;
    _activeDeviceId = null;
    tempHistory.clear();
    humidityHistory.clear();
    notifyListeners();
  }

  @override
  void dispose() {
    disconnect();
    super.dispose();
  }
}
