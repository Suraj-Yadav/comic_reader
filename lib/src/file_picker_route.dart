import 'dart:io';

import 'package:comic_reader/src/comic.dart';
import 'package:comic_reader/src/comic_grid_route.dart';
import 'package:file_chooser/file_chooser.dart';
import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

class FilePickerRoute extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Choose Files"),
      ),
      body: Center(
        child: RaisedButton(
          onPressed: () async {
            final filePaths = [];
            if (Platform.isAndroid || Platform.isIOS) {
              var res = await FilePicker.getMultiFilePath();
              filePaths.addAll(
                  res.values.where((element) => element.endsWith('cbz')));
            } else {
              final chosenFiles = await showOpenPanel(
                  allowedFileTypes: ['cbz'], allowsMultipleSelection: true);
              if (!chosenFiles.canceled && chosenFiles.paths.length > 0) {
                filePaths.addAll(chosenFiles.paths);
              }
            }
            if (filePaths.isNotEmpty) {
              Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => ComicGridRoute(
                        filePaths.map((filePath) => Comic(filePath)).toList()),
                  ));
            }
          },
          child: Text('Open file picker'),
        ),
      ),
    );
  }
}
