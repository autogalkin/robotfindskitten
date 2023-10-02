
project(easing
        DESCRIPTION "a lib which provides easing functions from from the https://easings.net/ and https://github.com/nicolausYes/easing-functions"
        VERSION 0.1.0
        LANGUAGES CXX
        )

add_library(${PROJECT_NAME} INTERFACE easing.h)
