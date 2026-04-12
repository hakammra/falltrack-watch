import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'alerts_page.dart';

class ConnectPage extends StatefulWidget {
  const ConnectPage({super.key});

  @override
  State<ConnectPage> createState() => _ConnectPageState();
}

class _ConnectPageState extends State<ConnectPage> {
  final controller = TextEditingController();
  String status = "";

  Future<void> linkToDevice() async {
    String id = controller.text.trim();
    if (id.isEmpty) return;

    SharedPreferences prefs = await SharedPreferences.getInstance();
    await prefs.setString('linked_device_id', id);
    setState(() => status = "Device linked successfully!");

    await Future.delayed(const Duration(seconds: 2));
    Navigator.pushReplacement(context, MaterialPageRoute(builder: (_) => const AlertsPage()));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Link to ESP Device")),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            const Text("Enter device ID from main user"),
            TextField(
              controller: controller,
              decoration: const InputDecoration(labelText: "Device ID"),
            ),
            const SizedBox(height: 20),
            ElevatedButton(onPressed: linkToDevice, child: const Text("Link")),
            const SizedBox(height: 10),
            Text(status),
          ],
        ),
      ),
    );
  }
}
