import 'package:comic_reader/src/file_picker_route.dart';
import 'package:flutter/material.dart';

const PAGE_CHANGE_DURATION = 500;
const PAN_DURATION = 1000;

const EPSILON = 0.000001;

Iterable<List<T>> zip<T>(Iterable<Iterable<T>> iterables) sync* {
  if (iterables.isEmpty) return;
  final iterators = iterables.map((e) => e.iterator).toList(growable: false);
  while (iterators.every((e) => e.moveNext())) {
    yield iterators.map((e) => e.current).toList(growable: false);
  }
}

void main() {
  runApp(MaterialApp(
    title: 'Comic Reader',
    home: FilePickerRoute(),
  ));
}
