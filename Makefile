run:
	[ -d logs ] || mkdir logs
	cmake . -B build
	cmake --build build
	cp build/datscity ./datscity
	./datscity > logs/$$(date +"%Y%m%d%H%M").log 2> logs/$$(date +"%Y%m%d%H%M").err

git:
	git add .
	git commit -m "$(filter-out $@, $(MAKECMDGOALS))"
	git push origin main

%:
	@true