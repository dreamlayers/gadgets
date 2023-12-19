import sys, shutil

if len(sys.argv) != 3:
    print("Usage: pyinstall.py file.py /path/prefix")
    exit(1)

destdir = next(x for x in sys.path if x.startswith(sys.argv[2]))
shutil.copy(sys.argv[1], destdir)
print(sys.argv[1], "copied to", destdir)
