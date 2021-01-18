suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file)

df <- df %>%
    mutate(phi = recode_factor(phi, `0.01`="phi == 0.01", `0.001`="phi == 0.001"))

plot <-
    ggplot(df, aes(x=iteration, y=potential, color=graph)) +
    labs(y="Potential",
         x="Iteration",
         col="Graph type") +
    stat_summary(fun=median, geom="line") +
    scale_y_log10(labels = scales::number) +
    facet_grid(phi ~ ., label="label_parsed")

ggsave(output_file, plot)
