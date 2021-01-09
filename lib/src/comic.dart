import 'dart:io';

import 'package:archive/archive_io.dart';
import 'package:comic_reader/main.dart';
import 'package:flutter/material.dart';
import 'package:mime/mime.dart';
import 'package:path/path.dart' as path;

class ComicPage {
  final File imgFile;
  final Size size;

  ComicPage(this.imgFile, this.size);
}

Future<ComicPage> savePage(String filePath, List<int> bytes) async {
  final imageFile = File(filePath);
  await imageFile.create(recursive: true);
  await imageFile.writeAsBytes(bytes, flush: true);
  final decodedImage = await decodeImageFromList(bytes);
  return ComicPage(imageFile,
      Size(decodedImage.width.toDouble(), decodedImage.height.toDouble()));
}

class Comic {
  final String archiveFilePath;
  ComicPage firstPage;
  int numberOfPages;

  Comic({this.archiveFilePath});

  Future<bool> loadComicPreview() async {
    bool success = true;
    final archive =
        ZipDecoder().decodeBytes(await File(archiveFilePath).readAsBytes());

    final extractedCache = Directory(path.join(
        CACHE_DIRECTORY.path, path.basenameWithoutExtension(archiveFilePath)));

    numberOfPages = archive.files.length;

    for (var file in archive.files) {
      if (!file.isFile) {
        continue;
      }
      final mimeType = lookupMimeType(file.name, headerBytes: file.content);

      if (mimeType.startsWith("image/")) {
        firstPage = await savePage(
            extractedCache.path + "_Cover" + path.extension(file.name),
            file.content);
        break;
      }
    }
    return success;
  }
}
