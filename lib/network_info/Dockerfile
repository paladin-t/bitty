FROM jystic/centos6-ghc7.10.1

RUN cabal update
ADD . /opt/network-info
RUN cd /opt/network-info # && cabal install -j4

# Default Command for Container
WORKDIR /opt/network-info
CMD ["test/run-tests.sh"]
#CMD ["bash", "-lc", "cabal repl"]
