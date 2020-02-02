import 'dart:io';

import 'package:flutter/material.dart';

import 'package:archive/archive_io.dart';
import 'package:mime/mime.dart';

class ComicPage {
  final Image page;
  Size size;

  ComicPage(this.page) {
    this.page.image.resolve(ImageConfiguration()).addListener(
        ImageStreamListener((ImageInfo imageInfo, bool synchronousCall) {
      this.size = Size(
          imageInfo.image.width.toDouble(), imageInfo.image.height.toDouble());
    }));
  }
}

class Comic {
  List<ComicPage> pages;
  final String filePath;
  final ComicPage firstPage;

  factory Comic(String filePath) {
    final archive = ZipDecoder().decodeBytes(File(filePath).readAsBytesSync());
    for (var file in archive.files) {
      if (file.isFile) {
        final mimeType = lookupMimeType(file.name, headerBytes: file.content);
        if (mimeType.startsWith('image/')) {
          return Comic._internal(
              filePath, ComicPage(Image.memory(file.content)));
        }
      }
    }
    return null;
  }

  Comic._internal(this.filePath, this.firstPage);

  void loadPages() {
    final archive = ZipDecoder().decodeBytes(File(filePath).readAsBytesSync());
    cleanPages();
    pages = [];
    for (var file in archive.files) {
      if (file.isFile) {
        final mimeType = lookupMimeType(file.name, headerBytes: file.content);
        if (mimeType.startsWith('image/')) {
          pages.add(ComicPage(Image.memory(file.content)));
        }
      }
    }
  }

  void cleanPages() {
    if (pages != null) {
      pages = null;
    }
  }
}
