<<<<<<< HEAD
# Microsystems for Assisted Living: Indoor Localization Helper

This repository contains the 2 platformio projects. One for the Tags (M5StickC) and one for the Anchor (M5Stack Core)



## Anchor-Tag routine description

      1. Scan and stay connected with all 3 tags
      2. onDisconnect do Anchor rescan
      3. when (nConnectedTags == nTags) -> Anchor stop scan.
      4. For tags on disconnect -> Tag start advertise, connected ->stop



## Getting Started 
Prerequisits:
* Install git on your computer

* Download Visual Studio Code and install the PlatformIO Extention

Setup:
1. Clone the repository your computer 
```
git clone https://github.com/andrew-elsayeh/MAL_WS202021_GroupA.git
```
2. Open the firmware folder you want to flash in VSCode 
  > Example: (Welcome Page -> Open Folder -> MAL_Tag_Firmare

3. While your board is connected, click on the flash icon from platformio (bottom left), platformio will do the rest. First time might take a while to install required tools for flashing


## Contributing Code

Please follow the following steps to contribute code in order to keep the repository tidy

Example: Implementing a new GUI Feature

1. Create a new branch for your feature 
```
git branch awesome_gui
```
2. Change your current branch into the new branch 
```
git checkout awesome_gui
```
3. Write your code 
```
/* 
 * Awesome new GUI Feature
 */
 void drawAwesomePicture(int pictureSize){
    //Something fancy
  }
```
4. When you're happy with your code, commit it (this makes a snapshot of the current state of the program so that you can revert to it later if things get messed up)
```
git add main.cpp #replace main.cpp with whatever file you changed or added or removed

git commit -m "Added drawAwesomePicture function that draws awesome pictures" #Change the message with a short discription of your change

```
5. If you want the rest of the team to see your branch use:
```
git push origin awesome_gui
```
6. This feature will only exist within this branch, to merge it with the main branch, consult with the rest of the team, as only one person should be responsible with merging.



=======
>>>>>>> ble_dac_control

