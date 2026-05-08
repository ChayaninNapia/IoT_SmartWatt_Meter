import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';

import 'package:smart_watt_meter/main.dart';
import 'package:smart_watt_meter/providers/app_state.dart';

void main() {
  testWidgets('App renders bottom navigation shell', (WidgetTester tester) async {
    await tester.pumpWidget(
      ChangeNotifierProvider(
        create: (_) => AppState(),
        child: const SmartWattMeterApp(),
      ),
    );

    expect(find.text('Device'), findsWidgets);
    expect(find.text('Power'), findsOneWidget);
    expect(find.text('Map'), findsOneWidget);
    expect(find.text('History'), findsOneWidget);
  });
}
