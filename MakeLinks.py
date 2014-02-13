import os
import os.path
from subprocess import call

def makeLinks(dllFolder, coherentFolder, includePDBandLIB = False):
	fullDllFolder = os.path.abspath(dllFolder)
	files = os.listdir(fullDllFolder)

	for f in files:
		splitted = os.path.splitext(f)
		extensions = [".dll"]
		if includePDBandLIB:
			extensions.append(".pdb")
			extensions.append(".lib")
		if splitted[1] in extensions:
			call('mklink "%s" "%s"' % (os.path.join(coherentFolder, f), os.path.join(fullDllFolder, f)), shell=True)

dllFolder32 = "..\\Voxels\\src\\bin32"
buildFolder32 = "bin32\\"
makeLinks(dllFolder32, buildFolder32)

dllFolder64 = "..\\Voxels\\src\\bin64"
buildFolder64 = "bin64\\"
makeLinks(dllFolder64, buildFolder64)
