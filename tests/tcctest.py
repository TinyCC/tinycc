import subprocess
import sys
import difflib

def main():
  reference = subprocess.check_output([sys.argv[1]])
  compare = subprocess.check_output(sys.argv[2:])
  failed = False
  for line in difflib.unified_diff(reference.split('\n'), compare.split('\n'), fromfile='cc', tofile='tcc', lineterm=''):
    failed = True
    print line
  sys.exit(1 if failed else 0)
  
if __name__ == '__main__':
  main()
