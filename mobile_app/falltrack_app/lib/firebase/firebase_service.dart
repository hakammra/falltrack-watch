import 'package:http/http.dart' as http;
import 'dart:convert';

class FirebaseService {
  static const String baseUrl = 'https://falltrack-default-rtdb.firebaseio.com/devices/B7K4Z2/alerts.json';

  static Future<List<String>> getAlerts() async {
    final response = await http.get(Uri.parse(baseUrl));
    if (response.statusCode == 200) {
      final data = jsonDecode(response.body);
      if (data == null) return [];
      return (data as Map).values.map((e) => e['timestamp'].toString()).toList();
    }
    return [];
  }
}