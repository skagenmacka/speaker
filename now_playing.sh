#!/usr/bin/env bash

# Exempel: du måste själv sätta NAME baserat på den data librespot skickar.
# Om du får JSON på stdin:
# NAME=$(cat | jq -r '.common_metadata_fields.name')

# Om du får env-variabler:
# NAME="$TRACK_NAME"

if [ -z "$NAME" ]; then
  exit 0
fi

curl -s -X POST "http://localhost:8080/now_playing?name=${NAME}" >/dev/null
