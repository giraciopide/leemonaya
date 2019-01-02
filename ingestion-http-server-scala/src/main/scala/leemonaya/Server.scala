package leemonaya

import cats.effect.{ConcurrentEffect, Effect, ExitCode, IO, IOApp, Timer}
import cats.implicits.toFunctorOps
import org.http4s.HttpRoutes
import org.http4s.server.Router
import org.http4s.server.blaze.BlazeServerBuilder
import org.http4s.syntax.kleisli.http4sKleisliResponseSyntax

object Server extends IOApp {
  def run(args: List[String]): IO[ExitCode] =
    ServerStream.stream[IO].compile.drain.as(ExitCode.Success)
}

object ServerStream {
  def routes[F[_]: Effect]: HttpRoutes[F] = new ServerRoutes[F].routes

  def stream[F[_]: ConcurrentEffect: Timer]: fs2.Stream[F, ExitCode]=
    BlazeServerBuilder[F]
      .bindHttp(8080, "0.0.0.0")
      .withHttpApp(Router(
        "/" -> routes
      ).orNotFound)
      .serve
}
