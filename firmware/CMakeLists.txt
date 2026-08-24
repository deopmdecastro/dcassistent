cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(dc_firmware)

# Cria a imagem SPIFFS `dcweb` a partir de `frontend-preview/` e regista-a
# como partição gerada, para ser flashada por:
#   idf.py dcweb-flash          # só o SPIFFS
#   idf.py flash                # tudo (app + SPIFFS)
#
# Detalhes:
#  - PARTITION dcweb --> ver partitions.csv (2 MB @ 0x620000)
#  - SPIFFS_IMAGE_FLASH_IN_PROJECT: assinala como parte de `flash`
#  - a pasta origem `frontend-preview/` contém o `index.html` já com
#    responsividade adaptada ao ecrã 240x320 do ES3C28P.
spiffs_create_partition_image(
    dcweb
    ${CMAKE_SOURCE_DIR}/../frontend-preview
    FLASH_IN_PROJECT
)
