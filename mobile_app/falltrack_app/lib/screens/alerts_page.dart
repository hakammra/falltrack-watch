import 'package:flutter/material.dart';
import '../firebase/firebase_service.dart';

class AlertsPage extends StatelessWidget {
  const AlertsPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Alerts")),
      body: FutureBuilder<List<String>>(
        future: FirebaseService.getAlerts(),
        builder: (context, snapshot) {
          if (!snapshot.hasData) return const Center(child: CircularProgressIndicator());
          final alerts = snapshot.data!;
          return ListView.builder(
            itemCount: alerts.length,
            itemBuilder: (_, i) => ListTile(title: Text(alerts[i])),
          );
        },
      ),
    );
  }
}
