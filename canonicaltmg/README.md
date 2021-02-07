# canonicaltmg

**Purpose:**<br>
Reorganize .tmg data deterministically, to produce DIFFable files.

**Usage:**<br>
`canonicaltmg [-t NumThreads] <InputPath> <OutputPath>`
* If `-t` is not specified or is invalid, canonicaltmg will attempt to hog all the cores.
* If canonicaltmg can't figure out how many threads to run automatically, it will run only 1.
* If `InputPath` is a directory, canonicaltmg converts every .tmg file therein, writing to the `OutputPath` directory.
* If `InputPath` is a regular file, then `OutputPath` is treated as the output path & filename.
