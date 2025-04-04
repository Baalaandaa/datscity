run:
	cmake . -B build
	cmake --build build
	cp build/datscity
	./datscity

git:
	git add .
	git commit -m "$(filter-out $@, $(MAKECMDGOALS))"
	git push origin main

%:
	@true