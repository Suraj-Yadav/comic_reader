import 'dart:io';

import 'package:comic_reader/src/comic_gallery_route.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

class FilePickerRoute extends StatelessWidget {
  const FilePickerRoute({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    if (Platform.isMacOS || Platform.isWindows || Platform.isLinux) {
      windowManager.setTitle("Choose Files");
    }
    return Scaffold(
      appBar: AppBar(
        title: const Text("Choose Files"),
      ),
      body: Center(
        child: ElevatedButton(
          style: ElevatedButton.styleFrom(padding: const EdgeInsets.all(20)),
          onPressed: () async {
            final List<String> filePaths = [];
            var res =
                (await FilePicker.platform.pickFiles(allowMultiple: true))!;
            for (var path in res.paths) {
              if (path != null && path.endsWith('cbz')) {
                filePaths.add(path);
              }
            }

            if (filePaths.isEmpty) {
              return;
            }
            if (context.mounted) {
              Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => ComicGalleryRoute(filePaths),
                  ));
            }
          },
          child: const Text('Open file picker'),
        ),
      ),
    );
  }
}
