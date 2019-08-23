#!/bin/bash

export GOPATH=`pwd`
export PATH=$GOPATH/bin:$PATH:

YEAR=`date "+%y"`
MONTH=`date "+%m"`
DAY=`date "+%d"`

function build_bin() {
	
	APP_VERSION="V$YEAR.$MONTH.$DAY"
	APP_BUILD_TIME=`date "+%Y%m%d%H%M%S"`

	GOOS=linux GOARCH=amd64 go build -ldflags	\
		"-X main.appVersion=$APP_VERSION -X main.appBuildTime=$APP_BUILD_TIME"	\
		-o main main.go
}

build_bin 
