import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/device_service.dart';

class SettingsPage extends StatelessWidget {
  const SettingsPage({super.key});

  Future<void> resetAll() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.clear();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Settings")),
      body: ListView(
        children: [
          ListTile(title: const Text("Main Device ID"), subtitle: FutureBuilder(
            future: DeviceService.getDeviceId(),
            builder: (context, snapshot) => Text(snapshot.data?.toString() ?? "Loading..."),

          )),
          ListTile(title: const Text("Reset All Data"), onTap: resetAll),
        ],
      ),
    );
  }
}