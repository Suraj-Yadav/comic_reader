// import 'dart:ffi';
// import 'dart:io';
// import 'dart:math';

// import 'package:ffi/ffi.dart';
// import 'package:flutter/foundation.dart';
// import 'package:mime/mime.dart' as mime;
// import 'package:path/path.dart';

// import 'generated_bindings.dart';

// int archiveVersionNumber() => _bindings.archive_version_number();

// String toDartString(Pointer<Char> ptr) => ptr.cast<Utf8>().toDartString();

// String _getError(Pointer<archive> archivePtr) =>
//     toDartString(_bindings.archive_error_string(archivePtr));

// /// An exception thrown when there was a problem in the archive library.
// class ArchiveException extends FormatException {
//   ArchiveException(String message, [dynamic source, int? offset])
//       : super(message, source, offset);
// }

// class ArchiveFile {
//   final Pointer<archive> _archivePtr;
//   final Pointer<archive_entry> _entryPtr;

//   late final Uint8List headerBytes;

//   ArchiveFile(this._entryPtr, this._archivePtr) {
//     if (isFile) {
//       final length = min(mime.defaultMagicNumbersMaxLength, size);

//       final buffer = malloc.allocate<Uint8>(length);
//       final len =
//           _bindings.archive_read_data(_archivePtr, buffer.cast(), length);
//       if (len < 0) {
//         throw ArchiveException(
//             "Error reading input archive: ${_getError(_archivePtr)}");
//       }

//       headerBytes = Uint8List(length);
//       for (var i = 0; i < length; i++) {
//         headerBytes[i] = buffer.asTypedList(length)[i];
//       }

//       malloc.free(buffer);
//     } else {
//       headerBytes = Uint8List(0);
//     }
//   }

//   String get path => toDartString(_bindings.archive_entry_pathname(_entryPtr));

//   int get size => _bindings.archive_entry_size(_entryPtr);

//   int get _type => _bindings.archive_entry_filetype(_entryPtr);
//   bool get isFile => _type == AE_IFREG;

//   void writeContent(String filePath) {
//     final file = File(filePath);
//     file.createSync(recursive: true);
//     final fp = file.openSync(mode: FileMode.write);
//     fp.writeFromSync(headerBytes);

//     const buffLength = 8192;
//     final buffer = malloc.allocate<Uint8>(buffLength);
//     var len =
//         _bindings.archive_read_data(_archivePtr, buffer.cast(), buffLength);
//     while (len > 0) {
//       fp.writeFromSync(buffer.asTypedList(buffLength), 0, len);
//       len = _bindings.archive_read_data(_archivePtr, buffer.cast(), buffLength);
//     }
//     fp.flushSync();
//     fp.closeSync();
//     if (len < 0) {
//       file.deleteSync();
//       throw ArchiveException(
//           "Error reading input archive: ${_getError(_archivePtr)}");
//     }
//   }
// }

// Iterable<ArchiveFile> getArchiveFiles(String path) sync* {
//   final archivePtr = _bindings.archive_read_new();
//   if (archivePtr == nullptr) {
//     throw ArchiveException("Couldn't create archive reader");
//   }
//   if (_bindings.archive_read_support_filter_all(archivePtr) != ARCHIVE_OK) {
//     throw ArchiveException("Couldn't enable decompression");
//   }
//   if (_bindings.archive_read_support_format_all(archivePtr) != ARCHIVE_OK) {
//     throw ArchiveException("Couldn't enable read formats");
//   }
//   if (_bindings.archive_read_open_filename(
//           archivePtr, path.toNativeUtf8().cast(), 0) !=
//       ARCHIVE_OK) {
//     throw ArchiveException("Unable to read file: ${_getError(archivePtr)}");
//   }

//   Pointer<Pointer<archive_entry>> entryAddr = malloc.allocate(1);
//   while (true) {
//     final r = _bindings.archive_read_next_header(archivePtr, entryAddr);
//     if (r == ARCHIVE_EOF) {
//       break;
//     } else if (r != ARCHIVE_OK) {
//       throw ArchiveException(
//           "Unable to read next header: ${_getError(archivePtr)}");
//     }
//     yield ArchiveFile(entryAddr.value, archivePtr);
//   }
//   malloc.free(entryAddr);

//   _bindings.archive_read_free(archivePtr);
// }

// String _getAssetsPath(String path) {
//   if (kReleaseMode) {
//     // I'm on release mode, absolute linking
//     final String assetsPath = join('data', 'flutter_assets', 'assets', path);
//     return join(Directory(Platform.resolvedExecutable).parent.path, assetsPath);
//   } else {
//     // I'm on debug mode, local linking
//     return join(Directory.current.path, 'assets', path);
//   }
// }

// const String _libName = 'archive';

// /// The dynamic library in which the symbols for [LibarchiveBindings] can be found.
// final DynamicLibrary _dylib = () {
//   if (Platform.isMacOS) {
//     for (var path in [
//       '/opt/homebrew/opt/libarchive/lib/libarchive.dylib',
//       '/usr/local/opt/libarchive/lib/libarchive.dylib'
//     ]) {
//       if (File(path).existsSync()) {
//         return DynamicLibrary.open(path);
//       }
//     }
//   }
//   if (Platform.isAndroid || Platform.isLinux) {
//     return DynamicLibrary.open('lib$_libName.so');
//   }
//   if (Platform.isWindows) {
//     return DynamicLibrary.open(
//         _getAssetsPath('third_party/libarchive/bin/archive.dll'));
//   }
//   throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
// }();

// /// The bindings to the native functions in [_dylib].
// final LibarchiveBindings _bindings = LibarchiveBindings(_dylib);
