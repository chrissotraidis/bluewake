import zipfile
z = zipfile.ZipFile("/home/james/bw-build-android/apk-training/unsigned.apk")
infos = sorted(z.infolist(), key=lambda i: i.header_offset)
for i in infos:
    print(i.header_offset, i.compress_type, i.file_size, i.filename)
