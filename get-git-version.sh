#!/bin/bash

#git log \
#    --pretty="format:%ci, author: \"%aN\" <%aE>, commit: %h" -1 "${1}" || \
#    echo no git \

# TODO: esse arquivo deve ter um "export hook", ou algo assim, que faça
#           com que ele seja substituído por um simples "echo versão".
#       alternativa: criar um commit hook que substitui esse arquivo por
#           um "echo versão" -> nao funciona, pq nao da pra saber o hash
#           antes de commitar

git describe || printf "%s\n" "<unknown version>"
