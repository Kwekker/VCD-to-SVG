
text="$1"
echo "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"><text id=\"text1\" font-size=\"48\" font-family=\"DejaVu Sans\">$text</text></svg>" >| /tmp/rustiek.svg
inkscape --pipe \
  --export-filename=/tmp/floep \
  --export-type=svg \
  --query-id=text1 \
  --query-width \
  /tmp/rustiek.svg