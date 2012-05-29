# UrbiLoquendoASR #



## INFO ##

Urbi module for Loquendo ASR  

## LICENSE ##

Copyright (C) 2012  Lukasz Malek
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
  
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

## REQUIREMENTS ##

## INSTALATION ##

This library is prepared to work with the cmake.
Create new directory
```
mkdir build
```
then run cmake, ccmake or cmake-gui to configure project
```
ccmake ../
```
after configuring project type
```
make
```

## MODULE FUNCTIONS ##

**ULoquendoASR.init(1);**            - initialize module   

## USAGE ##
```
urbi -i -m ULoquendoASR -P 54000

var Global.ASR=ULoquendoASR.new(1);
```
## NOTES ##

The test of the application can be done in the folowing way
```
var Global.ASR=ULoquendoASR.new(1);

at(ASR.newRecognizedText?([var text, var confidence])){
   echo("Recognized: " + text + " confidence=" + confidence);
};

at(ASR.newRecognizedText?(var pl)){
   echo("Recognized: "+ pl);
};

at(ASR.newRecognizedText?){
   echo("Recognized: ");
};