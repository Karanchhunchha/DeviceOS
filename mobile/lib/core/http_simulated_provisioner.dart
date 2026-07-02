import 'dart:convert';
import 'package:http/http.dart' as http;
import 'provisioning_transport.dart';

class HttpSimulatedProvisioner implements ProvisioningTransport {
  @override
  Future<void> provision({
    required String target,
    required String ssid,
    required String password,
    required String tenantId,
  }) async {
    // Normalize target (e.g. "9001" -> "http://10.101.104.32:9001/provision")
    String host = target;
    if (!host.contains(':')) {
      host = '10.101.104.32:$host';
    }
    if (!host.startsWith('http://') && !host.startsWith('https://')) {
      host = 'http://$host';
    }

    final url = Uri.parse('$host/provision');
    
    try {
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({
          'ssid': ssid,
          'password': password,
          'tenant_id': tenantId,
        }),
      );

      if (response.statusCode != 200) {
        final errorMsg = response.body.isNotEmpty ? response.body : 'Status Code: ${response.statusCode}';
        throw Exception('Simulated provisioning failed: $errorMsg');
      }
    } catch (e) {
      throw Exception('Could not connect to simulated device at $url. Error: $e');
    }
  }
}
