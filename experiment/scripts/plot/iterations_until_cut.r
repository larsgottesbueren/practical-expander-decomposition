suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

# lm.fit <- lm(iterations ~ log10_squared_edges, data=df)
# summary(lm.fit)

plot <-
    ggplot(df, aes(x=log10_squared_edges, y=iterations)) +
    geom_point(alpha=0.5, aes(color=graph, shape=factor(random_walk_steps))) +
    expand_limits(x=0, y=0) +
    labs(y="Iterations",
         x=expression("log"^2 ~ "m"),
         col="Graph type",
         shape="Random walk steps")

ggsave(output_file, plot)
