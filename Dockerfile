# Build stage
FROM eclipse-temurin:17-jdk-jammy AS build
WORKDIR /app
COPY . .
RUN mkdir -p engine/build
RUN javac -d engine/build engine/src/main/java/com/optiflow/engine/*.java

# Run stage
FROM eclipse-temurin:17-jre-jammy
WORKDIR /app
COPY --from=build /app/engine/build ./engine/build
COPY --from=build /app/data ./data
EXPOSE 5000
CMD ["java", "-cp", "engine/build", "com.optiflow.engine.OptiFlowServer"]
