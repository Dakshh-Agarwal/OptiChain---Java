# Build stage
FROM openjdk:17-slim AS build
WORKDIR /app
COPY . .
RUN mkdir -p engine/build
RUN javac -d engine/build engine/src/main/java/com/optiflow/engine/*.java

# Run stage
FROM openjdk:17-slim
WORKDIR /app
COPY --from=build /app/engine/build ./engine/build
COPY --from=build /app/data ./data
EXPOSE 5000
CMD ["java", "-cp", "engine/build", "com.optiflow.engine.OptiFlowServer"]
