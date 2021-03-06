args <- commandArgs(trailingOnly = TRUE)

install <- FALSE
travis <- FALSE
if (length(args) == 0) {
  stop(paste0(
    "\nRequired arguments:\n",
    "\t(1) path to R folder.\n",
    "\t(2) Optionally, boolean whether to install packages.\n",
    "\t(3) Optionally, additional checks for Travis."
  ))
} else if (length(args) == 1) {
  lib <- args[1]
} else if (length(args) == 2) {
  lib <- args[1]
  install <- ifelse(tolower(args[2]) == "true", TRUE, FALSE)
} else if (length(args) == 3) {
  lib <- args[1]
  install <- ifelse(tolower(args[2]) == "true", TRUE, FALSE)
  travis <- ifelse(tolower(args[3]) == "true", TRUE, FALSE)
} else {
  stop(paste0(
    sprintf("\nExpected one, two or three arguments, got %d arguments.\n", length(args)),
    "\nRequired arguments:\n",
    "\t(1) path to R folder.\n",
    "\t(2) Optionally, boolean whether to install packages.\n",
    "\t(3) Optionally, additional checks for Travis."
  ))
}

installed <- installed.packages()
INSTALL_opts <- c("--no-docs", "--no-html", "--no-multiarch")
options("repos" = "https://cloud.r-project.org")

if (travis) {

  if (!"devtools" %in% installed)
    install.packages("devtools", INSTALL_opts = INSTALL_opts)

  # install jasptools if necessary
  if (!"jasptools" %in% installed)
    devtools::install("Tools/jasptools/", upgrade = "always", quick = TRUE)

  # create ~/.R/makevars for C/   C++ compilation flags
  dir.create("~/.R")
  fileConn <- file("~/.R/Makevars")
  writeLines(
    paste(
      "CFLAGS   += -O0",
      "CXXFLAGS += -O0",
      sep = "\n"
    ), 
    fileConn
  )
  close(fileConn)

  # a number of environment variables are only available on travis.
  # see here for an overview: https://docs.travis-ci.com/user/environment-variables/

  how <- Sys.getenv("TRAVIS_EVENT_TYPE")

  # get changed files
  diff <- system("git diff --name-only @~..@", intern = TRUE)

  # ignore some things that may appear inside the diff
  diff <- diff[!startsWith(diff, "warning: CRLF will be replaced by LF")]
  diff <- diff[diff != "The file will have its original line endings in your working directory."]

  # store the changed files for linting
  saveRDS(diff[endsWith(diff, ".R")], file = "modifiedRfiles.rds")

  cat(sprintf("\nTravis understood that the following files where modified in this %s:\n\n %s\n",
              how, paste0(diff, collapse = "\n")))

  # check some additional dependencies on travis
  # note that jaspResults should be installed before setwd(lib)!
  if (!"jaspResults" %in% installed || any(startsWith(diff, "JASP-R-Interface/jaspResults/")))
    install.packages("JASP-R-Interface/jaspResults/", repos=NULL, type="source", INSTALL_opts = INSTALL_opts)

  if (!"BH" %in% installed)
    install.packages("BH")

}

if (dir.exists(lib)) {
  if (any(endsWith(list.files(lib), ".R"))) {
    setwd(lib)
  } else {
    stop("Could not find any R files in the directory")
  }
} else {
  stop("Could not find directory")
}



pkgs <- c("stringr", "testthat")
for (pkg in pkgs[! pkgs %in% installed]) {
  install.packages(pkg)
}

library(stringr)

basePkgs <- installed.packages(priority="high")
basePkgs <- basePkgs[basePkgs[, "Priority"] == "base", 1]

reqPkgs <- NULL
# Expression to match namespace in [valid namespace -> triple or double colons -> valid function name]:
expr <- '([a-zA-Z0-9.]{2,}(?<![.]))(?:::|:::)[a-zA-Z0-9._]+'
comment <- '#.*'
files <- list.files(getwd(), pattern="\\.[Rr]$")
for (file in files) {
  content <- suppressWarnings(readLines(file))
  content <- gsub(comment, "", content) # remove comments
  matches <- stringr::str_match_all(content, expr)
  matches <- unlist(lapply(matches, function(match) match[, 2]))
  if (length(matches) == 0)
    next
  reqPkgs <- c(reqPkgs, matches)
}

# for some reason, RcppArmadillo is not picked up as dependency
# but it definitely needs to be installed before other packages.
if (! "RcppArmadillo" %in% installed) {
  install.packages("RcppArmadillo", INSTALL_opts = INSTALL_opts)
}

# Temporarly add the GPArotation manually (incorrectly marked as "Suggest' in psych)
reqPkgs <- c(reqPkgs, "GPArotation")
reqPkgs <- reqPkgs[!reqPkgs %in% 'JASPgraphs']
# Exclude jasptools manually (should not be shipped)
basePkgs <- c(basePkgs, "jasptools")
reqPkgs <- sort(unique(reqPkgs))
reqPkgs <- reqPkgs[! reqPkgs %in% basePkgs]

if (install) {
  pkgs2install <- reqPkgs[! reqPkgs %in% installed]
  if (length(pkgs2install) > 0) {
    cat("Installing all missing packages...")
    for (pkg in pkgs2install) {
      install.packages(pkg, repos = 'https://cloud.r-project.org', dependencies = c("Depends", "Imports"),
                       INSTALL_opts = INSTALL_opts)
    }
    cat("\nFinished iterating over the required packages\n")
  } else {
    cat("\nAll required packages are available from cache.\n")
  }
} else {
  strPkgs <- paste0("'", reqPkgs, "'")
  installString <- paste0("install.packages(c(", paste(strPkgs, collapse=", "), "), repos = 'https://cloud.r-project.org', dependencies = c('Depends', 'Imports'))")

  deps <- tools::package_dependencies(reqPkgs, recursive=TRUE, which=c('Depends', 'Imports'))
  depPkgs <- unlist(deps)
  depPkgs <- sort(unique(depPkgs))

  allPkgs <- sort(unique(c(reqPkgs, depPkgs)))

  cat("\nInstall string:\n")
  cat(installString)
  cat("\n\nRequired packages:\n")
  cat(paste0(reqPkgs, collapse="\n"), "\n")
  cat("\nDependencies of required packages [Imports, Depends]:\n")
  cat(paste0(depPkgs, collapse="\n"), "\n")
  cat("\nFull list of packages:\n")
  cat(paste0(allPkgs, collapse="\n"), "\n")
}

if (travis) {
  old <- installed[, c("Version"), drop = FALSE]
  installedPost <- installed.packages()
  new <- installedPost[, c("Version"), drop = FALSE]
  diffPkg <- !rownames(new) %in% rownames(old)
  toShow <- rbind(old, new[diffPkg, , drop = FALSE])
  toShow <- cbind(toShow, rep(c("Cache", "Installed"), c(nrow(old), sum(diffPkg))))
  colnames(toShow) <- c("Version", "From")

	msg <- cat("\nAVAILABLE PACKAGES\n")
	print(toShow)
}
