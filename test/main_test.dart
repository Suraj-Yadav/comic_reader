import 'dart:io';

import 'package:archive/archive.dart' as archive;
import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as path;

testFunc(String archivePath) {
  final filesExpected = {"a/b.txt": 1, "c.txt": 2, "test.png": 11645};

  final filesFound = {};
  final tempDir = Directory.systemTemp.createTempSync();
  for (var file in archive.getArchiveFiles(archivePath)) {
    if (file.isFile) {
      filesFound[file.path] = file.size;
      file.writeContent(path.join(tempDir.path, file.path));

      File tempFile = File(path.join(tempDir.path, file.path));
      expect(file.size, tempFile.lengthSync(),
          reason:
              "Size for '${file.path}' didn't match with '${tempFile.path}'");
    }
  }
  expect(filesFound, filesExpected);
  tempDir.deleteSync(recursive: true);
}

void main() {
  test('Test Rar', () {
    testFunc('./test/test.rar');
  });
  test('Test Zip', () {
    testFunc('./test/test.zip');
  });
}
