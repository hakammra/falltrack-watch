import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'dashboard_page.dart';
import 'alerts_page.dart';
import 'wifi_setup_page.dart';

class LoadingScreen extends StatefulWidget {
  const LoadingScreen({super.key});

  @override
  State<LoadingScreen> createState() => _LoadingScreenState();
}

class _LoadingScreenState extends State<LoadingScreen> {
  @override
  void initState() {
    super.initState();
    determineNavigation();
  }

  Future<void> determineNavigation() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    String? mainDeviceId = prefs.getString('main_device_id');
    String? thisDeviceId = prefs.getString('device_id');
    String? espConnected = prefs.getString('esp_connected'); // optional flag
    bool hasCredentials = prefs.getBool('wifi_saved') ?? false;

    await Future.delayed(const Duration(seconds: 2)); // simulate loading

    if (!hasCredentials) {
      Navigator.pushReplacement(context, MaterialPageRoute(builder: (_) => const WiFiSetupPage()));
    } else if (thisDeviceId == mainDeviceId) {
      Navigator.pushReplacement(context, MaterialPageRoute(builder: (_) => const DashboardPage()));
    } else {
      Navigator.pushReplacement(context, MaterialPageRoute(builder: (_) => const AlertsPage()));
    }
  }

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      body: Center(child: CircularProgressIndicator()),
    );
  }
}
