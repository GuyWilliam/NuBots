FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

# Install packages needed for the build scripts
RUN apt-get update \
    && apt-get install -y sudo software-properties-common git wget clang-format-6.0 colordiff \
    && apt-add-repository -y ppa:rael-gc/rvm \
    && apt-get update \
    && apt-get install -y rvm \
    && apt-get autoremove --purge

# Add puppet and build scripts to the image
RUN mkdir -p /NUbots-build/toolchain/
ADD ./puppet /NUbots-build/puppet/
ADD ./.travis /NUbots-build/.travis/

# Install the NUbots toolchain
RUN TRAVIS_BUILD_DIR=/NUbots-build /NUbots-build/.travis/install_toolchain.sh

# Remove extraneous files from the toolchain install
RUN rm -rf /NUbots-build \
    && apt-get autoremove --purge

# Create the directory that will be used for checkouts and builds
RUN mkdir -p /code/NUbots

# Create the nubots user and group with id 1000. This will be used to map to
# the user with id 1000 on the host (typically the first non-root user)
# Used to fix permission issues
RUN groupadd -g 1000 nubots \
    && useradd -r -u 1000 -g nubots nubots

# Give the nubots user ownership of the /code directory,
# and give all users read, write, and execute permissions
# Used to fix permission issues
RUN chown -R nubots:nubots /code \
    && chmod a+rwx -R /code

# Use the nubots user if no user is specified with `docker run`
USER nubots

# Run bash if no command is specified with `docker run`
CMD ["bash"]
