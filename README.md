<p align="left">
    <img src="https://img.shields.io/badge/version-1.0.1-brightgreen">
</p>

This library is a command-line program and can mount a Google Cloud Storage bucket to your local file system on Windows. It aims to provide the similar functionalities as [gcsfuse][gcsfuse] on Windows. Users can browse and read the files on cloud as-if the files are on disk. Note that GCSDokan is still under development, please feel free to open an issue if you see any unexpected results. 

[gcsfuse]: https://github.com/GoogleCloudPlatform/gcsfuse

## Dependencies & Installation
### Installing the binary version(release version)
You must install [Dokan driver and library][Dokan] before using the program. The release version of GCSDokan can be found [here][release]. After you have downloaded the program, Simply putting the path to the program to your environment variable `PATH`. Then you can check and run the program by typing `GCSDokan` in your terminal. If the Dokan driver has been properly installed, you should be able to see the help page of the program.

[release]: https://github.com/Jiefei-Wang/GCSDokan/releases

### Building the program from source
You need the following dependencies to build the program:

* [Dokan driver and library][Dokan]
* [stlcache][stlcache]
* [Boost filesystem][Boost]
* [Google Cloud CPP][Google Cloud CPP] (version > 1.16.0)
* Visual Studio

It is recommanded to compile the program using Visual Studio 2019 for the program has been written and tested on this platform. If you are using [vcpkg](vcpkg) to manage your libraries. The boost filesystem and Google Cloud CPP can be automatically linked by vcpkg when compiling GCSDokan. As of August 2020 you must install the HEAD version of Google Cloud CPP in vcpkg since the release version does not fulfull the version requirement of GCsDokan.

For building the program, the library stlcache and Dokan can be found via the environment variable `stlcache_root` and `dokan_root` respectively. For example, these environments on my machine are `C:\dev\library\include` and `C:\Program Files\Dokan\DokanLibrary-1.4.0`. If you do not use vcpkg, you have to manually add the pathes of the boost library and Google Cloud CPP to the project.


[Dokan]: https://dokan-dev.github.io/
[stlcache]: https://github.com/akashihi/stlcache
[Boost]: https://www.boost.org/
[Google Cloud CPP]: https://github.com/googleapis/google-cloud-cpp
[vcpkg]: https://github.com/microsoft/vcpkg

## Authentication
The program use a service account to authenticate with google. The credentials file can be found from either the program argument `--key <path>` or the environment variable `GOOGLE_APPLICATION_CREDENTIALS`. Please refer to [Google Authentication][] for how to create and use a service account.


[Google Authentication]: https://cloud.google.com/docs/authentication/production

## Usage
The use of the GCSDokan is simple, the basic command is
```
GCSDokan remote_path local_mount_point
```
For example, if you want to mount the bucket `genomics-public-data` to your local driver `Z`, you can type
```
GCSDokan genomics-public-data Z
```
Alternatively, you can use the URI of the bucket
```
GCSDokan gs://genomics-public-data Z
```
The `remote_path` does not have to be a bucket, you can mount the subdirectory `clinvar` of the bucket `genomics-public-data` as well
```
GCSDokan genomics-public-data/clinvar Z
```
similarly, you can mount a bucket to a folder under an existing disk given that the disk supports it.
```
GCSDokan genomics-public-data Z:\my_folder
```
For other options, please refer to the program help page `GCSDokan -h`.

## Limitation
* The program only supports reading files from the cloud. File writting is still under development

* For performance reason, folders and files information will be cached for a certain period of time. Therefore, if there are any changes in a bucket before the cached information has been expired, you would not be able to see the changes. It is highly suggested not trying to change the files in a bucket while using GCSDokan.  

* Random access will be costly due to the latancy of the network connection. GCSDokan will try its best to identify the random access and avoid the need of creating a local cache.

## Miscellaneous
* If there exists a file and a folder with the same name in the same directory, a tailing underscore will be added to the file name

* If you use a [placeholder][placeholder] file to maintain the directory structure in a bucket, the file will be named as ".placeholder" and have the `hidden` attribute.

[placeholder]: https://cloud.google.com/storage/docs/gsutil/addlhelp/HowSubdirectoriesWork

## future plan

* Support the file writting.

* Showing the correct bucket size.
