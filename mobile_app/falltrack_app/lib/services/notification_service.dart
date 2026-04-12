import 'package:flutter_local_notifications/flutter_local_notifications.dart';

class NotificationService {
  static final FlutterLocalNotificationsPlugin _plugin = FlutterLocalNotificationsPlugin();

  static Future<void> init() async {
    const settings = AndroidInitializationSettings('@mipmap/ic_launcher');
    await _plugin.initialize(const InitializationSettings(android: settings));
  }

  static Future<void> showAlert(String message) async {
    const details = AndroidNotificationDetails('fall_alerts', 'Fall Alerts', importance: Importance.max);
    await _plugin.show(0, 'FallTrack Alert', message, const NotificationDetails(android: details));
  }
}