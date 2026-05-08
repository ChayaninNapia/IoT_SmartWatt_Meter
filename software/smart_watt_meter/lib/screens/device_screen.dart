import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/app_state.dart';

class DeviceScreen extends StatelessWidget {
  const DeviceScreen({super.key});

  Color _statusColor(String status) {
    if (status.startsWith('connected')) return Colors.green;
    if (status == 'scanning' || status == 'connecting' || status == 'disconnecting') {
      return Colors.orange;
    }
    if (status.startsWith('error')) return Colors.red;
    return Colors.grey;
  }

  IconData _statusIcon(String status) {
    if (status.startsWith('connected')) return Icons.bluetooth_connected;
    if (status == 'scanning' || status == 'connecting' || status == 'disconnecting') {
      return Icons.bluetooth_searching;
    }
    if (status.startsWith('error')) return Icons.error_outline;
    return Icons.bluetooth_disabled;
  }

  @override
  Widget build(BuildContext context) {
    final app = context.watch<AppState>();
    final status = app.bleStatus;
    final isConnected = status == 'connected';
    final isBusy =
        status == 'scanning' || status == 'connecting' || status == 'disconnecting';

    return Scaffold(
      backgroundColor: const Color(0xFF0D1117),
      appBar: AppBar(
        backgroundColor: const Color(0xFF161B22),
        title: const Text('Device', style: TextStyle(color: Colors.white)),
        elevation: 0,
      ),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Status card
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(20),
              decoration: BoxDecoration(
                color: const Color(0xFF161B22),
                borderRadius: BorderRadius.circular(16),
                border: Border.all(
                  color: _statusColor(status).withValues(alpha: 0.4),
                  width: 1.5,
                ),
              ),
              child: Column(
                children: [
                  Icon(
                    _statusIcon(status),
                    size: 56,
                    color: _statusColor(status),
                  ),
                  const SizedBox(height: 12),
                  Text(
                    isConnected ? 'ESP32_PowerMeter_01' : 'No Device',
                    style: const TextStyle(
                      color: Colors.white,
                      fontSize: 18,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 6),
                  Text(
                    status.toUpperCase(),
                    style: TextStyle(
                      color: _statusColor(status),
                      fontSize: 13,
                      letterSpacing: 1.2,
                    ),
                  ),
                ],
              ),
            ),

            if (isConnected) ...[
              const SizedBox(height: 16),
              _InfoTile(label: 'Protocol', value: 'BLE GATT'),
              const SizedBox(height: 8),
              _InfoTile(
                label: 'Service UUID',
                value: '12345678-1234-1234-\n1234-123456789012',
              ),
              const SizedBox(height: 8),
              _InfoTile(label: 'Notify interval', value: '1 Hz'),
              const SizedBox(height: 8),
              _InfoTile(
                label: 'Device ID',
                value: app.bleService.deviceId ?? '—',
              ),
            ],

            const Spacer(),

            SizedBox(
              width: double.infinity,
              height: 52,
              child: isConnected
                  ? OutlinedButton.icon(
                      onPressed: isBusy ? null : app.disconnect,
                      icon: const Icon(Icons.bluetooth_disabled),
                      label: const Text('Disconnect'),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: Colors.redAccent,
                        side: const BorderSide(color: Colors.redAccent),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                    )
                  : ElevatedButton.icon(
                      onPressed: isBusy ? null : app.scanAndConnect,
                      icon: isBusy
                          ? const SizedBox(
                              width: 18,
                              height: 18,
                              child: CircularProgressIndicator(
                                strokeWidth: 2,
                                color: Colors.white,
                              ),
                            )
                          : const Icon(Icons.bluetooth_searching),
                      label: Text(
                        isBusy ? status.toUpperCase() : 'Scan & Connect',
                      ),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFF238636),
                        foregroundColor: Colors.white,
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(12),
                        ),
                      ),
                    ),
            ),
          ],
        ),
      ),
    );
  }
}

class _InfoTile extends StatelessWidget {
  final String label;
  final String value;
  const _InfoTile({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      decoration: BoxDecoration(
        color: const Color(0xFF161B22),
        borderRadius: BorderRadius.circular(10),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(color: Colors.grey, fontSize: 13)),
          Text(
            value,
            textAlign: TextAlign.right,
            style: const TextStyle(color: Colors.white70, fontSize: 13),
          ),
        ],
      ),
    );
  }
}
