# Using container on NERSC

NERSC supports podman-hpc, derived from podman — an open-source container framework.

## Building containers

The instructions for what to include in the container are in `Containerfile`. Build directly on Perlmutter:

```bash
podman-hpc build -t dfhydro:1.0 .
```

The `-t` flag sets the image name and tag (here `dfhydro:1.0`). If the `Containerfile` is elsewhere or has a different name:

```bash
podman-hpc build -f /path/to/file/MyContainerfile -t dfhydro:1.0 .
```

To list images:

```bash
podman-hpc images
```

If built successfully you will see something like:

```
REPOSITORY                TAG         IMAGE ID      CREATED         SIZE    
localhost/dfhydro         1.0         29d8a84d03f5  26 seconds ago  1.52 GB 
```

The image is created on the login node and will disappear when your session ends. To make it usable in jobs, migrate it:

```bash
podman-hpc migrate dfhydro:1.0
```

After migration the image list will show a read-only entry and might look like:

```
REPOSITORY                TAG         IMAGE ID      CREATED         SIZE        R/O
localhost/dfhydro         1.0         29d8a84d03f5  2 minutes ago  1.52 GB     false
localhost/dfhydro         1.0         3adf38c8d58b  2 minutes ago  1.52 GB     true
```

## Compiling MUSIC in the container

Run the container interactively and mount your workspace:

```bash
podman-hpc run -it --rm -v /global/homes/s/singhm/MUSIC_DF/MUSIC_Density_Frame/:/workspace dfhydro:1.0
```

- `-it` runs interactively.
- `--rm` removes the container after exit.
- `-v` mounts the host folder into the container at `/workspace`.

Inside the container the prompt may look like:

```
root@1ffc5ca94fb9:/workspace#
```
You can create a `build` folder and run `cmake`/`make` inside — required dependencies are included in the container.

## Running jobs using the container

When submitting MUSIC jobs, mount the libraries and set `LD_LIBRARY_PATH`. Example submission snippet:

```bash
HOST_LIB1=/global/homes/s/singhm/MUSIC_DF/MUSIC_Density_Frame/build/extern/DFHydro/DFHydro/
HOST_LIB2=/global/homes/s/singhm/MUSIC_DF/MUSIC_Density_Frame/build/src/

srun podman-hpc run --rm \
    -v $SLURM_SUBMIT_DIR:/work \
    -v ${HOST_LIB1}:${HOST_LIB1}:ro \
    -v ${HOST_LIB2}:${HOST_LIB2}:ro \
    -w /work \
    -e LD_LIBRARY_PATH="${HOST_LIB1}:${HOST_LIB2}:\$LD_LIBRARY_PATH" \
    localhost/dfhydro:1.0 \
    bash -c 'export LD_LIBRARY_PATH='"${HOST_LIB1}:${HOST_LIB2}:\$LD_LIBRARY_PATH"'; \
             time ./MUSIChydro music_input > log.dat'
```

- `HOST_LIB1` and `HOST_LIB2` are host library paths mounted read-only into the container.
- `MUSIChydro` is the executable and `music_input` is the parameter file.