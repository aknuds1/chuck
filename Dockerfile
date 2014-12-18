FROM aknudsen/emscripten:1.28.0

RUN rm /bin/sh && ln -s /bin/bash /bin/sh
ADD . /code
WORKDIR /code

RUN pushd emsdk_portable && source ./emsdk_env.sh && popd && pushd src && \
make clean && make -j5 CHUCK_DEBUG=1 CHUCK_EM_SOURCEMAP=1 \
CHUCK_EM_SAFEHEAP=3 emscripten
RUN cd js && grunt
