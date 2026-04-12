import 'package:flutter/material.dart';

class DashboardPage extends StatelessWidget {
  const DashboardPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Dashboard")),
      body: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Center(child: Text("ESP Connected to Wi-Fi")),
          ElevatedButton(onPressed: () => Navigator.pushNamed(context, '/alerts'), child: const Text("View Alerts")),
          ElevatedButton(onPressed: () => Navigator.pushNamed(context, '/settings'), child: const Text("Settings")),
        ],
      ),
    );
  }
}
