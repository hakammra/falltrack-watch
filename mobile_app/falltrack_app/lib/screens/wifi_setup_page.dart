import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';
import '../services/device_service.dart';

class WiFiSetupPage extends StatefulWidget {
  const WiFiSetupPage({super.key});

  @override
  State<WiFiSetupPage> createState() => _WiFiSetupPageState();
}

class _WiFiSetupPageState extends State<WiFiSetupPage> {
  final ssidController = TextEditingController();
  final passController = TextEditingController();
  String message = "";

  Future<void> sendCredentials() async {
    final ssid = ssidController.text.trim();
    final pass = passController.text.trim();
    if (ssid.isEmpty || pass.isEmpty) {
      setState(() => message = "Please enter both fields");
      return;
    }
    try {
      final uri = Uri.http('192.168.4.1', '/wifi');
      final response = await http.post(uri, body: "$ssid,$pass");
      if (response.statusCode == 200) {
        final prefs = await SharedPreferences.getInstance();
        final uuid = await DeviceService.getDeviceId();
        await prefs.setString('main_device_id', uuid);
        Navigator.pushReplacementNamed(context, '/dashboard');
      } else {
        setState(() => message = "Failed to send credentials");
      }
    } catch (_) {
      setState(() => message = "ESP not reachable. Are you on its Wi-Fi?");
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Setup Wi-Fi")),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            TextField(controller: ssidController, decoration: const InputDecoration(labelText: "SSID")),
            TextField(controller: passController, decoration: const InputDecoration(labelText: "Password"), obscureText: true),
            ElevatedButton(onPressed: sendCredentials, child: const Text("Send")),
            Text(message),
          ],
        ),
      ),
    );
  }
}
