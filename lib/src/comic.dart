import 'dart:io';

import 'package:archive/archive_io.dart';
import 'package:collection/collection.dart';
import 'package:comic_reader/main.dart';
import 'package:flutter/material.dart';
import 'package:image_size_getter/file_input.dart' as image_size_getter;
import 'package:image_size_getter/image_size_getter.dart' as image_size_getter;
import 'package:mime/mime.dart';
import 'package:path/path.dart' as path;

class ComicPage {
  final File imgFile;
  final Size size;

  ComicPage(this.imgFile, this.size);
}

ComicPage savePage(String filePath, ArchiveFile file) {
  final outputStream = OutputFileStream(filePath);
  file.writeContent(outputStream);
  outputStream.close();

  final imageFile = File(filePath);

  final s = image_size_getter.ImageSizeGetter.getSize(
      image_size_getter.FileInput(imageFile));
  return ComicPage(imageFile, Size(s.width.toDouble(), s.height.toDouble()));
}

class Comic {
  final String archiveFilePath;
  ComicPage firstPage;
  int numberOfPages;

  Comic({this.archiveFilePath});

  bool loadComicPreview() {
    bool success = true;
    final archive = ZipDecoder().decodeBuffer(InputFileStream(archiveFilePath));

    final extractedCache = Directory(path.join(
        cacheDirectory.path, path.basenameWithoutExtension(archiveFilePath)));

    numberOfPages = archive.files.length;

    archive.files.sort((a, b) => compareAsciiLowerCaseNatural(a.name, b.name));

    for (var file in archive.files) {
      if (!file.isFile) {
        continue;
      }
      final mimeType = lookupMimeType(file.name, headerBytes: file.content);

      if (mimeType != null && mimeType.startsWith("image/")) {
        firstPage = savePage(
            "${extractedCache.path}_Cover${path.extension(file.name)}", file);
        break;
      }
    }
    return success;
  }
}
