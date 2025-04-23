# backtest_interface

## Getting started

To get started with the backtest interface, follow the instructions below:

![Interface View](./images/interface_view.png)

``` 
mkdir -p ~/finance_repos && cd finance_repos
mv ~/ig-trading-bot/ ~/finance_repos
git clone http://10.8.0.1:9000/Maxxime/backtest_interface.git
cd backtest_interface && sudo docker compose up -d --build
```

Then you can view the backtest interface at this URL: 

```
http://localhost:9018
```

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/topics/git/add_files/#add-files-to-a-git-repository) or push an existing Git repository with the following command:

```
cd backtest_interface
git remote add origin http://10.8.0.1:9000/Maxxime/backtest_interface.git
git branch -M main
git push -uf origin main
```

### Authors 

This project was developed and maintained by Maxxime. 

## License
Private license

## Project status
I am currently hard working on this project.
I push updates almost every day to fit Alexandre's requirements
