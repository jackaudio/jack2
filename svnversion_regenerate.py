import subprocess
import sys
import os
import filecmp
import shutil

if len(sys.argv) != 2 and len(sys.argv) != 3:
    print("Usage: %s <file> [define_name]" % sys.argv[0])
    sys.exit(3)



REV = ""
FILENAME = os.path.join(os.getcwd(), sys.argv[1])
TEMPFILENAME = FILENAME+".tmp"

os.chdir(os.path.dirname(sys.argv[0]))

if len(sys.argv) == 3:
    DEFINE = sys.argv[2]
else:
    DEFINE = "SVN_VERSION"

if os.path.isdir(".git"):
    try:
        subprocess.check_output(["git", "status"])
    except subprocess.CalledProcessError:
        print("git status failed.")
        sys.exit(1)
    except EnvironmentError: 
        print("please check if git is installed and included in your path.")
        sys.exit(2)
    gitrevision = subprocess.check_output(["git", "rev-parse", "HEAD"])
    REV = "0+" + gitrevision.decode("ascii").rstrip("\n")
    gitdirty = subprocess.check_output(["git", "diff-index", "--name-only", "HEAD"])
    if gitdirty:
        REV += "-dirty"
else:
    REV = "unknown"

with open(TEMPFILENAME, "w") as file:
    file.write('#define %s "%s"\n' % (DEFINE, REV))
if os.path.isfile(FILENAME):
    if not filecmp.cmp(TEMPFILENAME, FILENAME):
        print("Regenerated %s (%s)" % (FILENAME, REV))
        shutil.copyfile(TEMPFILENAME, FILENAME)
    else:
        os.remove(TEMPFILENAME)
else:
    print("Generated %s (%s)" % (FILENAME, REV))
    shutil.copyfile(TEMPFILENAME, FILENAME)

os.remove(TEMPFILENAME)






