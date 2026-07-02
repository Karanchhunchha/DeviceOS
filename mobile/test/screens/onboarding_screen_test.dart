import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:deviceos_companion/screens/onboarding_screen.dart';

void main() {
  testWidgets('OnboardingScreen renders rationale and buttons', (WidgetTester tester) async {
    await tester.pumpWidget(const MaterialApp(home: OnboardingScreen()));

    expect(find.text('Welcome to DeviceOS Companion'), findsOneWidget);
    expect(find.text('Skip to Simulator Mode'), findsOneWidget);
    expect(find.text('Grant Permissions'), findsOneWidget);
  });
}
