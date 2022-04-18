VALEC_DIR="$1"
if [ "$VALEC_DIR" == "" ]; then
  echo "Please supply the valec directory."
  echo "Example: ~/ValeCompiler-0.1.3.3"
  exit
fi
shift

$VALEC_DIR/valec build vmdsitegen=src vmdparse=~/VmdParse/src parseiter=~/ParseIter/src --output_dir build --region_override resilient-v3 -o vmdsitegen $@
