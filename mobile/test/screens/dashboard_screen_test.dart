import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';
import 'package:deviceos_companion/main.dart';
import 'package:deviceos_companion/core/shadow_client.dart';

void main() {
  testWidgets('DashboardScreen renders all 3 modes and help dialog', (WidgetTester tester) async {
    await tester.pumpWidget(
      MultiProvider(
        providers: [
          ChangeNotifierProvider(create: (_) => ShadowClient(cloudHost: 'test_host')),
        ],
        child: const MaterialApp(home: DashboardScreen()),
      ),
    );

    // Verify 3 modes exist
    expect(find.text('Simulator Mode (HTTP)'), findsOneWidget);
    expect(find.text('Real BLE Mode'), findsOneWidget);
    expect(find.text('Host Peer Mode (Advertise)'), findsOneWidget);

    // Test Host Peer Mode tap
    await tester.tap(find.text('Host Peer Mode (Advertise)'));
    await tester.pump();
    expect(find.text('Advertising...'), findsOneWidget);
    
    // Open Help Dialog
    await tester.tap(find.byIcon(Icons.help_outline).first);
    await tester.pumpAndSettle();
    expect(find.text('How to Use This App'), findsOneWidget);
  });
}
