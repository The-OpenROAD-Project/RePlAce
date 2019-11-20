docker run -v $(pwd):/replace replace bash -c "ln -s /replace/build/replace /replace/test/"
docker run -v $(pwd):/replace replace bash -c "cd /replace/test && python3 regression.py run"
