import 'dart:io';

import 'package:comic_reader/src/libarchive/libarchive.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as path;

void main() {
  test('Simple Test', () {
    final filesExpected = {"a/b.txt": 1, "c.txt": 2, "test.png": 11645};

    testFunc(String archivePath) {
      final filesFound = {};
      final tempDir = Directory.systemTemp.createTempSync();
      for (var file in getArchiveFiles(archivePath)) {
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

    testFunc('./test/test.rar');
    testFunc('./test/test.zip');
  });
}
