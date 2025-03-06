


# Building:

```sh
docker build -t cpue .
docker run -d --rm -p1024:1024 cpue
# to get shell insight running container
docker exec -it <container_id> /bin/bash
```