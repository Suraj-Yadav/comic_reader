import 'dart:io';

import 'package:comic_reader/main.dart';
import 'package:archive/archive_io.dart';
import 'package:comic_reader/src/libarchive/libarchive.dart' as libarchive;
import 'package:flutter/material.dart';
import 'package:image_size_getter/file_input.dart' as image_size_getter;
import 'package:image_size_getter/image_size_getter.dart' as image_size_getter;
import 'package:mime/mime.dart' as mime;
import 'package:path/path.dart' as path;

class ComicPage {
  final File imgFile;
  final Size size;

  ComicPage(this.imgFile, this.size);
}

ComicPage savePage(String filePath, libarchive.ArchiveFile file) {
  file.writeContent(filePath);

  final imageFile = File(filePath);

  try {
    final s = image_size_getter.ImageSizeGetter.getSize(
        image_size_getter.FileInput(imageFile));
    return ComicPage(imageFile, Size(s.width.toDouble(), s.height.toDouble()));
  } on RangeError catch (_) {
    return ComicPage(imageFile, const Size(100, 100));
  }
}

ComicPage savePageOld(String filePath, ArchiveFile file) {
  final outputStream = OutputFileStream(filePath);
  file.writeContent(outputStream);
  outputStream.close();

  final imageFile = File(filePath);

  try {
    final s = image_size_getter.ImageSizeGetter.getSize(
        image_size_getter.FileInput(imageFile));
    return ComicPage(imageFile, Size(s.width.toDouble(), s.height.toDouble()));
  } on RangeError catch (_) {
    return ComicPage(imageFile, const Size(100, 100));
  }
}

class Comic {
  final String archiveFilePath;
  late ComicPage firstPage;
  late int numberOfPages;

  Comic({required this.archiveFilePath});

  void loadComicPreview() {
    final extractedCache = Directory(path.join(
        cacheDirectory.path, path.basenameWithoutExtension(archiveFilePath)));

    ComicPage? cover;

    var pageCount = 0;
    for (var file in libarchive.getArchiveFiles(archiveFilePath)) {
      if (!file.isFile) {
        continue;
      }
      final mimeType =
          mime.lookupMimeType(file.path, headerBytes: file.headerBytes);

      if (mimeType != null && mimeType.startsWith("image/")) {
        cover ??= savePage(
            "${extractedCache.path}_Cover${path.extension(file.path)}", file);
        pageCount++;
      }
    }
    numberOfPages = pageCount;
    if (cover == null) {
      throw const FormatException("Archive had no image file");
    }
    firstPage = cover;
  }
}
