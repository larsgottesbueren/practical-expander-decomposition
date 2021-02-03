suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))
suppressMessages(library(ggpmisc))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

# lm.fit <- lm(iterations ~ log10_squared_edges, data=df)
# summary(lm.fit)

f <- y ~ x

plot <-
    ggplot(df, aes(x=log10_squared_edges, y=iterations, group=factor(random_walk_steps), color=factor(random_walk_steps))) +
    geom_point() +
    geom_smooth(method="lm", formula=f, fullrange=TRUE) +
    stat_poly_eq(aes(label = ..eq.label..), formula=f, parse=TRUE) +
    expand_limits(x=0, y=0) +
    labs(y="Iterations",
         x=expression("log"^2 ~ "m"),
         col="Random walk steps")

ggsave(output_file, plot)
