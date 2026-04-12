import 'package:flutter/material.dart';
import 'screens/loading_screen.dart';       // Import LoadingScreen
import 'screens/wifi_setup_page.dart';
import 'screens/dashboard_page.dart';
import 'screens/alerts_page.dart';
import 'screens/settings_page.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const FallTrackApp());
}

class FallTrackApp extends StatelessWidget {
  const FallTrackApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'FallTrack',
      theme: ThemeData(primarySwatch: Colors.indigo),
      home: const LoadingScreen(),       // Start with LoadingScreen
      routes: {
        '/wifi_setup': (_) => const WiFiSetupPage(),
        '/dashboard': (_) => const DashboardPage(),
        '/alerts': (_) => const AlertsPage(),
        '/settings': (_) => const SettingsPage(),
      },
    );
  }
}
