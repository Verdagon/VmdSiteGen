VALEC_DIR="$1"
if [ "$VALEC_DIR" == "" ]; then
  echo "Please supply the valec directory."
  echo "Example: ./build.sh ~/ValeCompiler-0.1.3.3 ~/VmdParse ~/ParseIter"
  exit
fi
shift

VMD_PARSE_DIR="$1"
if [ "$VMD_PARSE_DIR" == "" ]; then
  echo "Please supply the VmdParse directory."
  echo "Example: ./build.sh ~/ValeCompiler-0.1.3.3 ~/VmdParse ~/ParseIter"
  exit
fi
shift

PARSE_ITER_DIR="$1"
if [ "$PARSE_ITER_DIR" == "" ]; then
  echo "Please supply the ParseIter directory."
  echo "Example: ./build.sh ~/ValeCompiler-0.1.3.3 ~/VmdParse ~/ParseIter"
  exit
fi
shift

$VALEC_DIR/valec build vmdsitegen=src vmdparse=$VMD_PARSE_DIR/src parseiter=$PARSE_ITER_DIR/src --output_dir build --region_override resilient-v3 -o vmdsitegen $@
