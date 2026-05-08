import 'package:cloud_firestore/cloud_firestore.dart';
import 'package:firebase_auth/firebase_auth.dart';
import '../models/ride_sample.dart';
import '../models/ride_summary.dart';

class FirestoreService {
  FirebaseFirestore? _db;
  FirebaseAuth? _auth;

  FirebaseFirestore get _firestore {
    _db ??= FirebaseFirestore.instance;
    return _db!;
  }

  FirebaseAuth get _firebaseAuth {
    _auth ??= FirebaseAuth.instance;
    return _auth!;
  }

  String? get uid => _auth?.currentUser?.uid;

  Future<void> signInAnonymously() async {
    if (_firebaseAuth.currentUser != null) return;
    final cred = await _firebaseAuth.signInAnonymously();
    final userId = cred.user!.uid;
    await _firestore.collection('users').doc(userId).set({
      'createdAt': FieldValue.serverTimestamp(),
      'lastActiveAt': FieldValue.serverTimestamp(),
    }, SetOptions(merge: true));
  }

  Future<void> uploadRide(RideSummary summary, List<RideSample> samples) async {
    final userId = uid;
    if (userId == null) throw Exception('Not authenticated');

    final rideRef = _firestore
        .collection('users')
        .doc(userId)
        .collection('rides')
        .doc(summary.rideId);

    final batch = _firestore.batch();
    batch.set(rideRef, summary.toFirestore());

    for (int i = 0; i < samples.length; i++) {
      final sampleRef = rideRef.collection('samples').doc('s_$i');
      batch.set(sampleRef, samples[i].toFirestore());
    }

    await batch.commit();

    await _firestore.collection('users').doc(userId).update({
      'lastActiveAt': FieldValue.serverTimestamp(),
    });
  }

  Future<List<RideSummary>> fetchRides() async {
    final userId = uid;
    if (userId == null) return [];

    final snap = await _firestore
        .collection('users')
        .doc(userId)
        .collection('rides')
        .orderBy('startTime', descending: true)
        .get();

    return snap.docs
        .map((d) => RideSummary.fromFirestore(d.id, d.data()))
        .toList();
  }
}
